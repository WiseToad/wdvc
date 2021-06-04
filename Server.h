#ifndef SERVER_H
#define SERVER_H

#include "Params.h"
#include "Timing.h"
#include "Scales.h"
#include "Stateful.h"
#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <memory>

    //-- class Server --//

class Server final: public ProcessStateful
{
public:
    void run();

private:
    static void onMediaConfigure0(
            GstRTSPMediaFactory * pFactory, GstRTSPMedia * pMedia, Server * pThis);
    void onMediaConfigure(
            GstRTSPMediaFactory * pFactory, GstRTSPMedia * pMedia);

    static void onNeedData0(
            GstAppSrc * pAppSrc, guint, Server * pThis);
    void onNeedData(
            GstAppSrc * pAppSrc);

    std::unique_ptr<Capturer> mpCapturer;
    std::unique_ptr<Scales> mpScales;
    std::unique_ptr<ScalesLogger> mpScalesLogger;
    std::unique_ptr<ScalesFilter> mpScalesFilter;
    GstClockTime mTimestamp;
    int mFrameSerial;
};

#endif // SERVER_H
