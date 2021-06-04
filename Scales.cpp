#include "Scales.h"
#include "Msg.h"
#include "Blend.h"
#include "Params.h"
#include "Guard.h"
#include "Common.h"
#include <iomanip>
#include <cmath>

    //-- class Scales --//

Scales::Scales():
    mRecoveryTimeout(3000), mIdleTimeout(3000), mReadBuf(1024)
{
}

void Scales::updateWeight()
{
    Msg(FILELINE, 3) << "Updating scales weight";

    if(state(State::invalid))
        recover();
    if(state(State::initial))
        init();
    if(state(State::valid))
        update();
}

void Scales::init()
{
    if(!state(State::initial)) {
        Msg(FILELINE) << "Invalid initial scales state";
        return;
    }

    ScopeGuard failureGuard([this]() {
        setState(State::invalid);
        mRecoveryTimeout.start();
    });

    Msg(FILELINE, 2) << "Opening COM port: COM" << Params()->comPort;
    mhComPort = CreateFile(
                Text<TCHAR>("COM") + std::to_string(Params()->comPort),
                GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if(mhComPort == INVALID_HANDLE_VALUE) {
        Msg(FILELINE) << "Could not open COM port, error " << GetLastError();
        return; // failureGuard
    }

    Msg(FILELINE, 2) << "Tuning COM port";
    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    DCB dcb = {};
    /**/
    DCB dcb;
    memset(&dcb, 0, sizeof(dcb));
    /**/
    dcb.DCBlength = sizeof(DCB);
    if(!GetCommState(mhComPort, &dcb)) {
        Msg(FILELINE) << "Could not retrieve current COM port settings, error "
                      << GetLastError();
        return; // failureGuard
    }
    dcb.fBinary         = 1;
    dcb.BaudRate        = CBR_9600;
    dcb.ByteSize        = 8;
    dcb.Parity          = NOPARITY;
    dcb.StopBits        = ONESTOPBIT;
    dcb.fOutxCtsFlow    = 0;
    dcb.fOutxDsrFlow    = 0;
    dcb.fDtrControl     = DTR_CONTROL_ENABLE;
    dcb.fDsrSensitivity = 0;
    dcb.fOutX           = 0;
    dcb.fInX            = 0;
    dcb.fNull           = 0;
    dcb.fRtsControl     = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError   = 1;
    if(!SetCommState(mhComPort, &dcb)) {
        Msg(FILELINE) << "Could not tune COM port, error " << GetLastError();
        return; // failureGuard
    }

    Msg(FILELINE, 2) << "Setting COM port timeouts";
    // Set timeouts so data will be read immediately with no waiting
    // at all, even if no any data came at the moment of reading
    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    COMMTIMEOUTS timeouts = {};
    /**/
    COMMTIMEOUTS timeouts;
    memset(&timeouts, 0, sizeof(timeouts));
    /**/
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    if(!SetCommTimeouts(mhComPort, &timeouts)) {
        Msg(FILELINE) << "Could not set COM port timeouts, error " << GetLastError();
        return; // failureGuard
    }

    Msg(FILELINE, 2) << "Setting COM port event mask";
    if(!SetCommMask(mhComPort, EV_BREAK | EV_CTS | EV_DSR | EV_ERR |
                    EV_RING | EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY)) {
        Msg(FILELINE) << "Could not set COM port event mask, error " << GetLastError();
        return; // failureGuard
    }

    Msg(FILELINE, 2) << "Creating sync event for COM port event";
    mhStatEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(!mhStatEvent) {
        Msg(FILELINE) << "Could not create sync event for COM port event, error "
                      << GetLastError();
        return; // failureGuard
    }

    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    mStatOverlapped = {};
    /**/
    memset(&mStatOverlapped, 0, sizeof(mStatOverlapped));
    /**/
    mStatOverlapped.hEvent = mhStatEvent;
    mStatWait = false;

    Msg(FILELINE, 2) << "Creating sync event for COM port overlapped reading";
    mhReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(!mhReadEvent) {
        Msg(FILELINE) << "Could not create sync event for COM port overlapped reading, error "
                      << GetLastError();
        return; // failureGuard
    }

    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    mReadOverlapped = {};
    /**/
    memset(&mReadOverlapped, 0, sizeof(mReadOverlapped));
    /**/
    mReadOverlapped.hEvent = mhReadEvent;
    mReadWait = false;

    mWeight = {};

    Msg(FILELINE, 2) << "COM port connection open";
    setState(State::valid);
    failureGuard.reset();

    mIdleTimeout.start();
}

void Scales::update()
{
    if(!state(State::valid)) {
        Msg(FILELINE) << "Invalid scales state";
        return;
    }

    ScopeGuard failureGuard([this]() {
        setState(State::invalid);
        mRecoveryTimeout.start();
    });

    DWORD comStatEvent;
    if(!mStatWait) {
        Msg(FILELINE, 3) << "Start detecting COM port events";
        if(!WaitCommEvent(mhComPort, &comStatEvent, &mStatOverlapped)) {
            if(GetLastError() != ERROR_IO_PENDING) {
                Msg(FILELINE) << "Could not detect COM port events, error "
                              << GetLastError();
                return; // failureGuard
            }
            Msg(FILELINE, 3) << "Waiting for overlapped COM port event detect completion";
            mStatWait = true;
        } else {
            Msg(FILELINE, 3) << "COM port event(s) detected, processing";
            updateStat(comStatEvent);
            ResetEvent(mhStatEvent);
        }
    }

    DWORD readCount;
    if(!mReadWait) {
        Msg(FILELINE, 3) << "Start reading COM port data";
        if(!ReadFile(mhComPort, mReadBuf.pData(), mReadBuf.count(), &readCount, &mReadOverlapped)) {
            if(GetLastError() != ERROR_IO_PENDING) {
                Msg(FILELINE) << "Could not read COM port data, error "
                              << GetLastError();
                return; // failureGuard
            }
            Msg(FILELINE, 3) << "Waiting for overlapped COM port data read completion";
            mReadWait = true;
        } else if(!readCount) {
            Msg(FILELINE, 3) << "Zero length COM port data has been read";
            ResetEvent(mhReadEvent);
        } else {
            Msg(FILELINE, 3) << "COM port data has been read, processing";
            updateData({mReadBuf.pData(), readCount});
            ResetEvent(mhReadEvent);
            mIdleTimeout.start();
        }
    }

    if(mStatWait || mReadWait) {
        Msg(FILELINE, 3) << "Checking overlapped COM port operation(s) status";
        HANDLE handles[] = {mhStatEvent, mhReadEvent};
        for(;;) {
            DWORD res = WaitForMultipleObjects(2, handles, FALSE, 0);
            if(res == WAIT_OBJECT_0) { // COM port event
                Msg(FILELINE, 3) << "Overlapped COM port event detect completed, retrieving result";
                if(!GetOverlappedResult(mhComPort, &mStatOverlapped, &comStatEvent, FALSE)) {
                    Msg(FILELINE) << "Could not retrieve overlapped COM port event detect result, error "
                                  << GetLastError();
                    return; // failureGuard
                }
                Msg(FILELINE, 3) << "COM port event(s) detected overlapped, processing";
                updateStat(comStatEvent);
                mStatWait = false;
                ResetEvent(mhStatEvent);
            } else if(res == WAIT_OBJECT_0 + 1) { // COM port data
                Msg(FILELINE, 3) << "Overlapped COM port data read completed, retrieving result";
                if(!GetOverlappedResult(mhComPort, &mReadOverlapped, &readCount, FALSE)) {
                    Msg(FILELINE) << "Could not retrieve overlapped COM port data read result, error "
                                  << GetLastError();
                    return; // failureGuard
                } else if(!readCount) {
                    Msg(FILELINE, 3) << "Zero length COM port data has been read overlapped";
                } else {
                    Msg(FILELINE, 3) << "COM port data has been read overlapped, processing";
                    updateData({mReadBuf.pData(), readCount});
                    mIdleTimeout.start();
                }
                mReadWait = false;
                ResetEvent(mhReadEvent);
            } else if(res == WAIT_TIMEOUT) {
                Msg(FILELINE, 3) << "No (more) COM port overlapped operations completed";
                break;
            } else {
                Msg(FILELINE) << "Could not check overlapped COM port operation(s) status, error "
                              << GetLastError();
                return; // failureGuard
            }
        }
    }

    failureGuard.reset();

    if(mIdleTimeout) {
        Msg(FILELINE, 2) << "No data from COM port for idle timeout, trying to reconnect";
        reset();
        init();
    }
}

void Scales::updateStat(DWORD comStatEvent)
{
    Msg(FILELINE, 3) << "COM port status event flags: " << comStatEvent;
    if(comStatEvent & EV_BREAK) {
        Msg(FILELINE) << "Break on input";
    }
    if(comStatEvent & (EV_CTS | EV_DSR | EV_RING | EV_RLSD)) {
        DWORD comSignalStat;
        if(!GetCommModemStatus(mhComPort, &comSignalStat)) {
            Msg(FILELINE) << "Could not retrieve COM port signal status";
        } else {
            std::stringstream ss;
            if(comSignalStat & MS_CTS_ON)
                ss << " CTS";
            if(comSignalStat & MS_DSR_ON)
                ss << " DSR";
            if(comSignalStat & MS_RING_ON)
                ss << " RING";
            if(comSignalStat & MS_RLSD_ON)
                ss << " RLSD";
            if(!ss.tellp())
                ss << " (none)";
            Msg(FILELINE, 2) << "COM port signals active:" << ss.str();
        }
    }
    if(comStatEvent & EV_ERR) {
        DWORD comErr;
        COMSTAT comStat;
        if(!ClearCommError(mhComPort, &comErr, &comStat)) {
            Msg(FILELINE) << "Could not retrieve COM port error status";
        } else {
            std::stringstream ss;
            if(comErr & CE_BREAK)
                ss << " BREAK";
            if(comErr & CE_FRAME)
                ss << " FRAME";
            if(comErr & CE_OVERRUN)
                ss << " OVERRUN";
            if(comErr & CE_RXOVER)
                ss << " RXOVER";
            if(comErr & CE_RXPARITY)
                ss << " RXPARITY";
            if(!ss.tellp())
                ss << " (none)";
            Msg(FILELINE) << "COM port errors occured:" << ss.str();

            ss.str("");
            if(comStat.fCtsHold)
                ss << " CTS hold";
            if(comStat.fDsrHold)
                ss << " DSR hold";
            if(comStat.fRlsdHold)
                ss << " RLSD hold";
            if(comStat.fXoffHold)
                ss << " XOFF hold";
            if(comStat.fXoffSent)
                ss << " XOFF sent";
            if(comStat.fEof)
                ss << " EOF";
            if(comStat.fTxim)
                ss << " TXIM";
            if(!ss.tellp())
                Msg(2) << " (none)";
            Msg(FILELINE, 2) << "COM port statuses:" << ss.str();

            Msg(FILELINE, 2) << "COM port input  queue size: " << comStat.cbInQue;
            Msg(FILELINE, 2) << "COM port output queue size: " << comStat.cbInQue;
        }
    }
    if(comStatEvent & EV_RXCHAR) {
        Msg(FILELINE, 2) << "Character(s) was received via COM port";
    }
    if(comStatEvent & EV_RXFLAG) {
        Msg(FILELINE, 2) << "Event character(s) was received via COM port";
    }
    if(comStatEvent & EV_TXEMPTY) {
        Msg(FILELINE, 2) << "Last character in the COM output buffer was sent";
    }
}

void Scales::updateData(const ByteBufferRef & data)
{
    Msg(FILELINE, 2) << "Received " << data.count << " bytes from COM port";
    Msg(FILELINE, 3) << "Data: \"" << std::string((char *)data.pData, data.count) << "\"";

    ByteBufferRef dataRef = data;
    ByteBufferRef tokenRef = {data.pData, 0};
    while(dataRef.count) {
        if(*dataRef.pData == '\r' || *dataRef.pData == '\n') {
            std::string token = mReadPending +
                    std::string((char *)tokenRef.pData, tokenRef.count);
            mReadPending.clear();
            if(!token.empty())
                parseLine(token);
            tokenRef += (tokenRef.count + 1);
        }
        ++tokenRef.count;
        ++dataRef;
    }
    mReadPending += std::string((char *)tokenRef.pData, tokenRef.count);
    if(mReadPending.length() > 50) {
        Msg(FILELINE, 2) << "Pending data get too large, bad COM port stream format?";
        mReadPending.clear();
    }
}

void Scales::parseLine(const std::string & line)
{
    Msg(FILELINE, 3) << "Extracted token: \"" << line << "\"";

    std::vector<std::string> tokens = splitLine(line, ',');
    if(tokens.size() == 3 || tokens.size() == 4) {
        if(tokens[0] == "ST") {
            mWeight.state = WeightState::stable;
        } else if(tokens[0] == "US") {
            mWeight.state = WeightState::unstable;
        } else if(tokens[0] == "OL") {
            mWeight.state = WeightState::overload;
        } else {
            mWeight = {};
        }
        if(mWeight.state != WeightState::unknown)
            parseWeight(tokens[tokens.size() - 1]);
        if(mWeight.state != WeightState::unknown) {
            Msg(FILELINE, 3) << "Weight value updated by protocol 1 or 2";
            return;
        }
    }

    tokens = splitLine(line, ' ');
    if(tokens.size() == 3) {
        if(tokens[1] == "W:") {
            mWeight.state = WeightState::stable;
        } else if(tokens[1] == "w:") {
            mWeight.state = WeightState::unstable;
        } else {
            mWeight = {};
        }
        if(mWeight.state != WeightState::unknown)
            parseWeight(tokens[2]);
        if(mWeight.state != WeightState::unknown) {
            Msg(FILELINE, 3) << "Weight value updated by protocol 3";
            return;
        }
    }
}

std::vector<std::string> Scales::splitLine(const std::string & line, char delim)
{
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> tokens;
    while(std::getline(ss, token, delim)) {
        token = trimToken(token);
        if(delim != ' ' || !token.empty())
            tokens.push_back(std::move(token));
    }
    return std::move(tokens);
}

std::string Scales::trimToken(const std::string & token)
{
    size_t firstPos = token.find_first_not_of(' ');
    if(firstPos == std::string::npos)
        return "";
    size_t lastPos = token.find_last_not_of(' ');
    return token.substr(firstPos, (lastPos - firstPos + 1));
}

void Scales::parseWeight(const std::string & token)
{
    Msg(FILELINE, 3) << "Parsing weight: \"" << token << "\"";

    size_t unitPos = token.find_first_not_of("0123456789.");
    if(unitPos == std::string::npos || !unitPos) {
        Msg(FILELINE, 3) << "Invalid weight token";
        mWeight.state = {};
        return;
    }

    std::string valueStr = token.substr(0, unitPos);
    std::string unitStr = trimToken(token.substr(unitPos));
    if(valueStr.empty() || unitStr.empty()) {
        Msg(FILELINE, 3) << "Invalid weight token";
        mWeight.state = {};
        return;
    }

    std::stringstream ss(valueStr.data());
    ss >> mWeight.value;

    if(unitStr == "g") {            // грамм
        mWeight.value *= 1;
    } else if(unitStr == "ct") {    // карат
        mWeight.value *= 0.1999694;
    } else if(unitStr == "lb") {    // фунт
        mWeight.value *= 453.59237;
    } else if(unitStr == "oz") {    // унция
        mWeight.value *= 28.349523125;
    } else if(unitStr == "dr") {    // драхма
        mWeight.value *= 1.7718451;
    } else if(unitStr == "GN") {    // гран
        mWeight.value *= 0.06479891;
    } else if(unitStr == "ozt") {   // унция тройская
        mWeight.value *= 31.1034768;
    } else if(unitStr == "dwt") {   // пеннивейт
        mWeight.value *= 1.55517384;
    } else if(unitStr == "MM") {    // момм
        mWeight.value *= 3.749996;
    } else if(unitStr == "tl.J") {  // тейл гонконгский ювелирный
        mWeight.value *= 37.4290018;
    } else if(unitStr == "tl.T") {  // тейл тайваньский
        mWeight.value *= 37.49995;
    } else if(unitStr == "tl.H") {  // тейл гонконгский
        mWeight.value *= 37.799375;
    } else if(unitStr == "t") {     // тола
        mWeight.value *= 11.6638038;
    } else {
        Msg(FILELINE, 3) << "Unknown weight unit";
        mWeight.state = {};
        return;
    }
    Msg(FILELINE, 3) << "Parsed succesfully";
}

void Scales::recover()
{
    if(!state(State::invalid) || !mRecoveryTimeout)
        return;

    Msg(FILELINE, 2) << "Recovering scales state";
    reset();
}

void Scales::reset()
{
    mhComPort.release();
    mhStatEvent.release();
    mhReadEvent.release();
    mReadPending.clear();

    setState(State::initial);
}

Scales::Weight Scales::weight() const
{
    return (state(State::valid) ? mWeight : Weight{});
}

    //-- class ScalesFilter --//

ScalesFilter::ScalesFilter(FrameSource * pSource, Scales * pScales):
    mpSource(pSource), mpScales(pScales)
{
}

Frame ScalesFilter::getFrame()
{
    Frame frame = mpSource->getFrame();

    if(!mpScales->state(Scales::State::valid))
        return frame;

    Msg(FILELINE, 3) << "Obtaining screen and info panel DC";
    WndHDC_Handle hScreenDC = WndHDC(NULL);
    HDC_Handle hInfoDC = CreateCompatibleDC(hScreenDC->hDC);
    if(!hScreenDC || !hInfoDC) {
        Msg(FILELINE) << "Could not obtain screen and info panel DC";
        return frame;
    }

    Frame infoFrame({150, 50});

    Msg(FILELINE, 3) << "Creating and selecting info panel bitmap";
    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    BITMAPINFOHEADER bitmapHdr = {};
    /**/
    BITMAPINFOHEADER bitmapHdr;
    memset(&bitmapHdr, 0,sizeof(bitmapHdr));
    /**/
    bitmapHdr.biSize = sizeof(BITMAPINFOHEADER);
    bitmapHdr.biWidth = infoFrame.size.width;
    bitmapHdr.biHeight = -infoFrame.size.height;
    bitmapHdr.biPlanes = 1;
    bitmapHdr.biBitCount = 32;
    bitmapHdr.biCompression = BI_RGB;

    HBITMAP_Handle hInfoBitmap = CreateDIBSection(
                hInfoDC, (BITMAPINFO*)&bitmapHdr,
                DIB_RGB_COLORS, (void**)&infoFrame.pPixels, NULL, 0);
    if(!hInfoBitmap || !infoFrame.pPixels) {
        Msg(FILELINE) << "Could not create info panel DC";
        return frame;
    }
    HGDIOBJ_Guard hOldBitmap = SelectObjectGuarded(hInfoDC, hInfoBitmap);
    if(!hOldBitmap) {
        Msg(FILELINE) << "Could not select info panel bitmap into DC";
        return frame;
    }

    memset(infoFrame.pPixels, 255, infoFrame.dataSize());

    Msg(FILELINE, 3) << "Creating and selecting info panel background brush";
    HBRUSH_Handle hBrush = CreateSolidBrush(RGB(32, 32, 32));
    if(!hBrush) {
        Msg(FILELINE) << "Could not create info panel background brush";
        return frame;
    }
    HGDIOBJ_Guard hOldBrush = SelectObjectGuarded(hInfoDC, hBrush);
    if(!hOldBrush) {
        Msg(FILELINE) << "Could not select info panel background brush into DC";
        return frame;
    }

    Msg(FILELINE, 3) << "Painting info panel background";
    if(!RoundRect(hInfoDC, 0, 0,
                  infoFrame.size.width, infoFrame.size.height, 13, 13)) {
        Msg(FILELINE) << "Could not paint info panel background";
        return frame;
    }

    Msg(FILELINE, 3) << "Creating info panel title font";
    HFONT_Handle hTitleFont = CreateFont(
                18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_MODERN, Text<TCHAR>("Courier New"));
    if(!hTitleFont) {
        Msg(FILELINE) << "Could not create info panel title font";
        return frame;
    }

    Msg(FILELINE, 3) << "Creating weight value font";
    HFONT_Handle hWeightFont = CreateFont(
                22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_MODERN, Text<TCHAR>("Courier New"));
    if(!hWeightFont) {
        Msg(FILELINE) << "Could not create weight value font";
        return frame;
    }

    SetBkMode(hInfoDC, TRANSPARENT);

    Msg(FILELINE, 3) << "Painting info panel title";
    SetTextColor(hInfoDC, RGB(200, 200, 200));
    HGDIOBJ_Guard hOldFont = SelectObjectGuarded(hInfoDC, hTitleFont);
    if(!hOldFont) {
        Msg(FILELINE) << "Could not select info panel title font into DC";
        return frame;
    }
    RECT textRect{4, 4, LONG(infoFrame.size.width) - 4,
                  LONG(infoFrame.size.height) / 2 - 2};
    if(!DrawTextEx(hInfoDC, (TCHAR *)Text<TCHAR>("Scales data").pStr(), -1,
                   &textRect, DT_CENTER | DT_TOP, NULL)) {
        Msg(FILELINE) << "Could not paint info panel title";
        return frame;
    }

    Scales::Weight weight = mpScales->weight();

    Msg(FILELINE, 3) << "Painting weight value";
    bool unknownState = false;
    if(weight.state == Scales::WeightState::stable) {
        SetTextColor(hInfoDC, RGB(224, 224, 224));
    } else if(weight.state == Scales::WeightState::unstable) {
        SetTextColor(hInfoDC, RGB(192, 192, 16));
    } else if(weight.state == Scales::WeightState::overload) {
        SetTextColor(hInfoDC, RGB(255, 16, 16));
    } else {
        Msg(FILELINE, 3) << "Could not paint weight value: Unknown weight state";
        unknownState = true;
    }
    if(!unknownState) {
        std::stringstream ss;
        if(weight.value > -10000.0 && weight.value < 10000.0)
            ss << std::fixed << std::setprecision(3)
               << weight.value << " g";
        else
            ss << "OVERFLOW";

        if(!SelectObject(hInfoDC, hWeightFont)) {
            Msg(FILELINE) << "Could not select weight value font into DC";
            return frame;
        }
        textRect = {
            4, LONG(infoFrame.size.height) / 2 - 2,
            LONG(infoFrame.size.width) - 4,
            LONG(infoFrame.size.height) - 4};
        if(!DrawTextEx(hInfoDC, (TCHAR *)Text<TCHAR>(ss.str()).pStr(), -1,
                       &textRect, DT_CENTER | DT_TOP, NULL)) {
            Msg(FILELINE) << "Could not paint weight value";
            return frame;
        }
    }

    Msg(FILELINE, 3) << "Adjusting info panel alpha";
    Pixel * p = infoFrame.pPixels;
    for(int i = infoFrame.size.area(); i > 0; --i, ++p)
        p->A = (p->A ? 0 : 216);

    Msg(FILELINE, 3) << "Calculating info panel position";
    FrameSize boundsSize{frame.size.width - infoFrame.size.width,
                         frame.size.height - infoFrame.size.height};
    FramePos panelPos = Params()->panelPos;
    if(panelPos.x < 0)
        panelPos.x += boundsSize.width;
    if(panelPos.y < 0)
        panelPos.y += boundsSize.height;
    panelPos.x = std::min(std::max(panelPos.x, 0), int(boundsSize.width));
    panelPos.y = std::min(std::max(panelPos.y, 0), int(boundsSize.height));

    Msg(FILELINE, 3) << "Blending info panel into frame";
    blendImage(frame.pLine(panelPos.y) + panelPos.x, frame.pitch,
               infoFrame.size.width, infoFrame.size.height,
               infoFrame.pPixels, infoFrame.pitch);

    return frame;
}

    //-- class ScalesLogger --//

ScalesLogger::ScalesLogger():
    mStopWriter(false), mWriterThread(ScalesLogger::writerMain, this)
{
}

ScalesLogger::~ScalesLogger()
{
    Msg(FILELINE, 2) << "Stopping weight log writer";
    mStopWriter = true;
    mWriterThread.join();
    Msg(FILELINE, 3) << "Weight log writer stopped";
}

void ScalesLogger::logWeight(const Scales::Weight & weight)
{
    if(mStopWriter ||
            weight.state != Scales::WeightState::stable ||
            fabs(weight.value - mPriorWeight.value) < 0.1)
        return;

    if(mLog.size() > 4096) {
        Msg(FILELINE) << "Weight log queue overflow with value "
                      << std::fixed << std::setprecision(3) << weight.value;
        return;
    }

    Msg(FILELINE, 2) << "Pushing into weight log queue value "
                     << std::fixed << std::setprecision(3) << weight.value;
    std::lock_guard<std::mutex> logMutexGuard(mLogMutex);
    mLog.push_back({weight, time(nullptr)});
    mPriorWeight = weight;
}

void ScalesLogger::writerMain()
{
    Msg(FILELINE, 3) << "Starting weight log writer";
    while(!mStopWriter) {
        bool empty;
        LogEntry entry;
        {
            Msg(FILELINE, 3) << "Reading weight log queue";
            std::lock_guard<std::mutex> logMutexGuard(mLogMutex);
            empty = mLog.empty();
            if(!empty) {
                entry = mLog.front();
                mLog.pop_front();
            }
        }
        if(empty) {
            Msg(FILELINE, 3) << "Weight log queue is empty";
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } else {
            while(!writeEntry(entry)) {
                for(int i = 0; i < 10 && !mStopWriter; ++i)
                    std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
}

bool ScalesLogger::writeEntry(const LogEntry & entry)
{
    if(mWriterFailed) {
        mhCon.release();
        mWriterFailed = false;
    }

    if(!mhCon) {
        Msg(FILELINE, 3) << "Initializing MySQL connection";
        mhCon = mysql_init(nullptr);
        if(!mhCon) {
            Msg(FILELINE) << "Could not initialize MySQL connection";
            mWriterFailed = true;
            return false;
        }
        mWriterConnected = false;
    }

    if(!mWriterConnected) {
        Msg(FILELINE, 3) << "Connecting to database";
        if(!mysql_real_connect(mhCon,
                    Params()->dbHost.data(),
                    Params()->dbUser.data(),
                    Params()->dbPass.data(),
                    nullptr,
                    Params()->dbPort,
                    nullptr, 0)) {
            Msg(FILELINE) << "Could not connect to database:\n"
                          << mysql_error(mhCon);
            mWriterFailed = true;
            return false;
        }
        mWriterConnected = true;
    }

    std::stringstream sql;
    sql << "call wdvc.log_weight("
        << std::fixed << std::setprecision(3) << entry.weight.value
        << ", current_user())";
    Msg(FILELINE, 3) << "Executing query:\n" << sql.str();
    if(mysql_query(mhCon, sql.str().data())) {
        Msg(FILELINE) << "Could not execute weight logging query:\n"
                      << mysql_error(mhCon);
        mWriterFailed = true;
        return false;
    }

    return true;
}
