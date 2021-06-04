#ifndef CAPTURER_H
#define CAPTURER_H

#include "Timing.h"
#include "Frame.h"
#include "Gdi.h"
#include "DirectX.h"
#include "Buffer.h"
#include <memory>

enum struct CapturerType {
    gdi, dx, null
};

    //-- class Capturer --//

class Capturer: public FrameSource
{
public:
    static std::unique_ptr<Capturer> create();

protected:
    Capturer() = default;

    Frame getNullFrame(const FrameSize & frameSize);
    void drawCursor(HDC hDC);

private:
    Buffer<Pixel> mFrameBuf;
};

    //-- class NullCapturer --//

class NullCapturer final: public Capturer
{
public:
    NullCapturer() = default;

    // deleted
    NullCapturer(const NullCapturer &) = delete;
    NullCapturer & operator = (const NullCapturer &) = delete;

    virtual Frame getFrame() override;

private:
    FrameSize mFrameSize;
};

    //-- class GdiCapturer --//

class GdiCapturer final: public Capturer
{
public:
    GdiCapturer();

    // deleted
    GdiCapturer(const GdiCapturer &) = delete;
    GdiCapturer & operator = (const GdiCapturer &) = delete;

    virtual Frame getFrame() override;

private:
    Timeout mRecoveryTimeout;
    WndHDC_Handle mhScreenDC;
    HDC_Handle mhBitmapDC;
    HBITMAP_Handle mhBitmap;
    Frame mFrame;
};

    //-- class DxCapturer --//

class DxCapturer final: public Capturer
{
public:
    DxCapturer();
    virtual ~DxCapturer() override;

    // deleted
    DxCapturer(const DxCapturer &) = delete;
    DxCapturer & operator = (const DxCapturer &) = delete;

    virtual Frame getFrame() override;

private:
    Timeout mRecoveryTimeout;
    IDirect3D9_Handle mhD3D;
    IDirect3DDevice9_Handle mhDevice;
    IDirect3DSurface9_Handle mhSurface;
    Frame mFrame;
    ByteBuffer mFrameBuf;
};

#endif // CAPTURER_H
