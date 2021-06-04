#include "Capturer.h"
#include "Msg.h"
#include "Params.h"
#include "Guard.h"
#include "SysHandler.h"
#include <iostream>

    //-- class Capturer --//

std::unique_ptr<Capturer> Capturer::create()
{
    switch(Params()->capturerType) {
        case CapturerType::gdi:
            Msg(FILELINE) << "Using GDI screen capturer";
            return std::make_unique<GdiCapturer>();
        case CapturerType::dx:
            Msg(FILELINE) << "Using DirectX screen capturer";
            return std::make_unique<DxCapturer>();
        case CapturerType::null:
            Msg(FILELINE) << "Using null screen capturer";
            return std::make_unique<NullCapturer>();
    }
    throw Err(FILELINE) << "No appropriate capturer";
}

Frame Capturer::getNullFrame(const FrameSize & frameSize)
{
    Msg(FILELINE, 3) << "Obtaining null frame";

    if(frameSize.area() != mFrameBuf.count()) {
        Msg(FILELINE, 2) << "(Re)creating null frame buffer";
        mFrameBuf = Buffer<Pixel>(frameSize.area());
    }

    /*
    Pixel * p = mFrameBuf;
    for(int i = frameSize.area(); i > 0; --i, ++p)
        *p = {64, 0, 0, 255};
    /**/
    memset(mFrameBuf, 64, mFrameBuf.size());

    return Frame(frameSize, mFrameBuf);
}

void Capturer::drawCursor(HDC hDC)
{
    Msg(FILELINE, 3) << "Obtaining cursor image";

    CURSORINFO curInfo;
    curInfo.cbSize = sizeof(CURSORINFO);
    if(!GetCursorInfo(&curInfo)) {
        Msg(FILELINE) << "GetCursorInfo failed, error " << GetLastError();
        return;
    }

    if(curInfo.flags == CURSOR_SHOWING) {
        ICONINFO iconInfo;
        if(!GetIconInfo(curInfo.hCursor, &iconInfo)) {
            Msg(FILELINE) << "GetIconInfo failed, error " << GetLastError();
            return;
        }
        DeleteObject(iconInfo.hbmMask);
        DeleteObject(iconInfo.hbmColor);

        if(!DrawIcon(hDC,
                     curInfo.ptScreenPos.x - iconInfo.xHotspot,
                     curInfo.ptScreenPos.y - iconInfo.yHotspot,
                     curInfo.hCursor)) {
            Msg(FILELINE) << "DrawIcon failed, error " << GetLastError();
            return;
        }
    }
}

    //-- class NullCapturer --//

Frame NullCapturer::getFrame()
{
    FrameSize frameSize{size_t(GetSystemMetrics(SM_CXSCREEN)),
                        size_t(GetSystemMetrics(SM_CYSCREEN))};

    return getNullFrame(frameSize);
}

    //-- class GdiCapturer --//

GdiCapturer::GdiCapturer():
    mRecoveryTimeout(3000)
{
}

Frame GdiCapturer::getFrame()
{
    FrameSize frameSize{size_t(GetSystemMetrics(SM_CXSCREEN)),
                        size_t(GetSystemMetrics(SM_CYSCREEN))};

    if(!mRecoveryTimeout)
        return getNullFrame(frameSize);

    if(!mhScreenDC) {
        Msg(FILELINE, 2) << "Obtaining screen DC";
        mhScreenDC = WndHDC(NULL);
        if(!mhScreenDC) {
            Msg(FILELINE) << "Could not obtain screen DC";
            mRecoveryTimeout.start();
            return getNullFrame(frameSize);
        }
    }

    if(mFrame.size != frameSize) {
        Msg(FILELINE, 2) << "New frame size, recreating capturer resources";
        mhBitmap.release();
        mhBitmapDC.release();
        mFrame = Frame(frameSize);
    }

    if(!mhBitmapDC) {
        Msg(FILELINE, 2) << "Creating bitmap DC";
        mhBitmapDC = CreateCompatibleDC(mhScreenDC->hDC);
        if(!mhBitmapDC) {
            Msg(FILELINE) << "Could not create bitmap DC";
            mRecoveryTimeout.start();
            return getNullFrame(frameSize);
        }
    }

    if(!mhBitmap) {
        Msg(FILELINE, 2) << "Creating bitmap";

        /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
        BITMAPINFOHEADER bitmapHdr = {};
        /**/
        BITMAPINFOHEADER bitmapHdr;
        memset(&bitmapHdr, 0, sizeof(bitmapHdr));
        /**/
        bitmapHdr.biSize = sizeof(BITMAPINFOHEADER);
        bitmapHdr.biWidth = frameSize.width;
        bitmapHdr.biHeight = -frameSize.height;
        bitmapHdr.biPlanes = 1;
        bitmapHdr.biBitCount = 32;
        bitmapHdr.biCompression = BI_RGB;

        mhBitmap = CreateDIBSection(mhBitmapDC, (BITMAPINFO*)&bitmapHdr,
                                    DIB_RGB_COLORS, (void **)&mFrame.pPixels, NULL, 0);
        if(!mhBitmap || !mFrame.pPixels) {
            Msg(FILELINE) << "Could not create bitmap";
            mRecoveryTimeout.start();
            return getNullFrame(frameSize);
        }
    }

    Msg(FILELINE, 3) << "Capturing screen image via GDI";
    HGDIOBJ_Guard hOldBitmap = SelectObjectGuarded(mhBitmapDC, mhBitmap);
    if(!hOldBitmap) {
        Msg(FILELINE) << "Could not select screen bitmap into DC";
        mRecoveryTimeout.start();
        return getNullFrame(frameSize);
    }
    if(!BitBlt(mhBitmapDC, 0, 0, frameSize.width, frameSize.height,
               mhScreenDC->hDC, 0, 0, SRCCOPY | CAPTUREBLT)) {
        if(GetLastError() == 6) {
            Msg(FILELINE) << "Restoring capturer resources";
            mhBitmap.release();
            mhBitmapDC.release();
            mhScreenDC.release();
            return getFrame();
        } else {
            Msg(FILELINE, 3) << "Could not capture screen image via GDI, error "
                             << GetLastError();
            // normal under some circumstances
            return getNullFrame(frameSize);
        }
    }

    drawCursor(mhBitmapDC);

    return mFrame;
}

    //-- class DxCapturer --//

DxCapturer::DxCapturer():
    mRecoveryTimeout(3000)
{
}

DxCapturer::~DxCapturer()
{
    // After Ctrl-C and SIGINT closing D3D handles lead to SIGSEGV
    if(SysHandler::isIntRequest()) {
        Msg(FILELINE, 2) << "Reset DX resources to prevent SIGSEGV";
        mhSurface.reset();
        mhDevice.reset();
        mhD3D.reset();
    }
}

Frame DxCapturer::getFrame()
{
    FrameSize frameSize{size_t(GetSystemMetrics(SM_CXSCREEN)),
                        size_t(GetSystemMetrics(SM_CYSCREEN))};

    if(!mRecoveryTimeout)
        return getNullFrame(frameSize);

    HRESULT hRes;

    if(!mhD3D) {
        Msg(FILELINE, 2) << "Obtaining D3D9 interface";
        mhD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if(!mhD3D) {
            Msg(FILELINE) << "Could not obtain D3D9 interface";
            mRecoveryTimeout.start();
            return getNullFrame(frameSize);
        }
    }

    if(mFrame.size != frameSize) {
        Msg(FILELINE, 2) << "New frame size, capturer resources will be recreated";
        mhSurface.release();
        mhDevice.release();
        mFrame = Frame(frameSize);
    }

    if(!mhDevice) {
        Msg(FILELINE, 2) << "Creating D3D Device";
        /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
        D3DPRESENT_PARAMETERS params = {};
        /**/
        D3DPRESENT_PARAMETERS params;
        memset(&params, 0, sizeof(params));
        /**/
        params.Windowed = TRUE;
        params.BackBufferCount = 1;
        params.BackBufferHeight = mFrame.size.height;
        params.BackBufferWidth = mFrame.size.width;
        params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        params.hDeviceWindow = NULL;

        hRes = mhD3D->CreateDevice(
                    D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
                    D3DCREATE_SOFTWARE_VERTEXPROCESSING, &params, &mhDevice);
        if(FAILED(hRes)) {
            Msg(FILELINE) << "Could not create D3D Device, error " << hRes;
            mRecoveryTimeout.start();
            return getNullFrame(frameSize);
        }
    }

    if(!mhSurface) {
        Msg(FILELINE, 2) << "Creating offscreen plain surface";
        hRes = mhDevice->CreateOffscreenPlainSurface(
                    mFrame.size.width, mFrame.size.height, D3DFMT_A8R8G8B8,
                    D3DPOOL_SYSTEMMEM, &mhSurface, nullptr);
        if(FAILED(hRes)) {
            Msg(FILELINE) << "Could not create offscreen plain surface, error " << hRes;
            mRecoveryTimeout.start();
            return getNullFrame(frameSize);
        }
    }

    Msg(FILELINE, 3) << "Capturing screen image via GDI";
    hRes = mhDevice->GetFrontBufferData(0, mhSurface);
    if(FAILED(hRes)) {
        Msg(FILELINE, 3) << "Could not capture screen image via DX, error " << hRes;
        // normal under some circumstances
        return getNullFrame(frameSize);
    }

    Msg(FILELINE, 3) << "Obtaining offscreen plain surface's DC";
    HDC hSurfaceDC;
    if(FAILED(mhSurface->GetDC(&hSurfaceDC))) {
        Msg(FILELINE) << "Could not obtain offscreen plain surface's DC";
    } else {
        ScopeGuard releaseDC([this, hSurfaceDC](){
            mhSurface->ReleaseDC(hSurfaceDC);
        });
        drawCursor(hSurfaceDC);
    }

    Msg(FILELINE, 3) << "Locking offscreen plain surface";
    D3DLOCKED_RECT lockRect;
    if(FAILED(mhSurface->LockRect(&lockRect, NULL, 0))) {
        Msg(FILELINE) << "Could not lock offscreen plain surface";
        mRecoveryTimeout.start();
        return getNullFrame(frameSize);
    }
    ScopeGuard unlockGuard([this](){
        mhSurface->UnlockRect();
    });

    mFrame.pitch = lockRect.Pitch;
    if(mFrameBuf.size() != mFrame.dataSize()) {
        Msg(FILELINE, 2) << "(Re)creating frame buffer";
        mFrameBuf = ByteBuffer(mFrame.dataSize());
        mFrame.pPixels = reinterpret_cast<Pixel *>(mFrameBuf.pData());
    }
    Msg(FILELINE, 3) << "Copying bits from surface into frame buffer";
    memcpy(mFrame.pPixels, lockRect.pBits, mFrame.dataSize());

    return mFrame;
}
