#include "Server.h"
#include "Msg.h"
#include "Common.h"
#include "Capturer.h"
#include "Guard.h"
#include "GStreamer.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <sys/time.h>

    //-- class Server --//

void Server::run()
{
    if(!state(State::initial)) {
        Msg(FILELINE) << "Invalid initial server state";
        return;
    }

    std::string gstDebug = "GST_DEBUG=" +
            std::to_string(Params()->gstTraceLevel);
    putenv(gstDebug.data());
    Msg(FILELINE, 3) << gstDebug;

    std::string gstPluginPath = "GST_PLUGIN_PATH=" +
            CharText(getExePath()) + "plugins";
    putenv(gstPluginPath.data());
    Msg(FILELINE, 3) << gstPluginPath;

    Msg(FILELINE, 2) << "Init GStreamer";
    gst_init(NULL, NULL);

    std::ostringstream ss;

    ss << "( appsrc name=desktopcapsrc ! videoconvert ! video/x-raw,format=I420";

    FrameSize frameSize = {size_t(GetSystemMetrics(SM_CXSCREEN)),
                           size_t(GetSystemMetrics(SM_CYSCREEN))};
    FrameSize encodeSize = frameSize.scaled(Params()->scale)
            .aligned(4).bounded({320, 200}, {1920, 1080});
    if(encodeSize != frameSize) {
        ss << " ! videoscale ! video/x-raw,width=" << encodeSize.width
           << ",height=" << encodeSize.height;
    }

    ss << " ! x264enc speed-preset=" << Params()->preset
       << " psy-tune=animation pass=qual quantizer=" << Params()->crf
       << " qp-max=" << Params()->crfMax
       << " key-int-max=" << Params()->keyint
       << " bitrate=" << (Params()->bitrate > 0 ? Params()->bitrate : 2048)
       << " intra-refresh=" << (Params()->intraRefresh ? "true" : "false");

    ss << " ! queue ! rtph264pay name=pay0 pt=96 perfect-rtptime=false config-interval=1 )";
    Msg(FILELINE, 2) << "Pipeline description: \"" << ss.str() << "\"";

    Msg(FILELINE, 2) << "Creating and setting up RTSP Media Factory";
    GstRTSPMediaFactory * pFactory = gst_rtsp_media_factory_new();
    g_signal_connect(pFactory, "media-configure", (GCallback)&onMediaConfigure0, this);
    gst_rtsp_media_factory_set_launch(pFactory, ss.str().data());
    gst_rtsp_media_factory_set_shared(pFactory, TRUE);

    Msg(FILELINE, 2) << "Creating and setting up RTSP Server";
    GstRTSPServer * pServer = gst_rtsp_server_new();
    gst_rtsp_server_set_service(pServer, std::to_string(Params()->rtspPort).data());
    if(!gst_rtsp_server_attach(pServer, NULL)) {
        Msg(FILELINE) << "Couldn't start RTSP Server";
        setState(State::failed);
        return;
    }

    Msg(FILELINE, 2) << "Setting up RTSP Server mount points";
    GstRTSPMountPoints_Handle hMounts = gst_rtsp_server_get_mount_points(pServer);
    gst_rtsp_mount_points_add_factory(hMounts, "/desktop", pFactory);

    Msg(FILELINE) << "Activated URL: rtsp://localhost:"
                  << Params()->rtspPort << "/desktop";

    Msg(FILELINE, 2) << "Creating and starting main loop";
    GMainLoop_Handle hMainLoop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(hMainLoop);

    setState(State::zombie);
}

void Server::onMediaConfigure0(
        GstRTSPMediaFactory * pFactory, GstRTSPMedia * pMedia, Server * pThis)
{
    pThis->onMediaConfigure(pFactory, pMedia);
}

void Server::onMediaConfigure(
        GstRTSPMediaFactory * pFactory, GstRTSPMedia * pMedia)
{
    Msg(FILELINE, 2) << "Configuring GStreamer media";

    Msg(FILELINE, 3) << "Obtaining GStreamer element from media";
    GstElement_Handle hElement = gst_rtsp_media_get_element(pMedia);
    Msg(FILELINE, 3) << "Obtaining GStreamer appsrc element";
    GstElement_Handle hAppSrc = gst_bin_get_by_name_recurse_up(
                GST_BIN((GstElement *)hElement), "desktopcapsrc");
    Msg(FILELINE, 3) << "Set GStreamer appsrc format";
    gst_util_set_object_arg(
                G_OBJECT((GstElement *)hAppSrc), "format", "time");

    FrameSize frameSize = {size_t(GetSystemMetrics(SM_CXSCREEN)),
                           size_t(GetSystemMetrics(SM_CYSCREEN))};
    Fps fps = Params()->fps;
    Msg(FILELINE, 3) << "Set GStreamer appsrc caps";
    g_object_set(G_OBJECT((GstElement *)hAppSrc), "caps",
                 gst_caps_new_simple(
                     "video/x-raw",
                     "format", G_TYPE_STRING, "BGRx",
                     "width", G_TYPE_INT, frameSize.width,
                     "height", G_TYPE_INT, frameSize.height,
                     "framerate", GST_TYPE_FRACTION, fps.num, fps.den,
                     NULL), NULL);

    Msg(FILELINE, 3) << "Connecting need-data signal";
    g_signal_connect(hAppSrc, "need-data", (GCallback)&onNeedData0, this);

    mTimestamp = 0;
    mFrameSerial = 0;

    Msg(FILELINE, 3) << "Configuring GStreamer media finished";
}

void Server::onNeedData0(
        GstAppSrc * pAppSrc, guint, Server * pThis)
{
    pThis->onNeedData(pAppSrc);
}

void Server::onNeedData(
        GstAppSrc * pAppSrc)
{
    Msg(FILELINE, 3) << "Data request for new frame: " << ++mFrameSerial;

    if(!mpCapturer) {
        Msg(FILELINE, 2) << "Creating capturer";
        mpCapturer = Capturer::create();
    }
    FrameSource * pFrameSource = mpCapturer.get();

    if(Params()->comPort) {
        if(!mpScales) {
            Msg(FILELINE, 2) << "Creating scales object";
            mpScales = std::make_unique<Scales>();
        }
        mpScales->updateWeight();

        if(!Params()->dbUser.empty()) {
            if(!mpScalesLogger) {
                Msg(FILELINE, 2) << "Creating scales logger";
                mpScalesLogger = std::make_unique<ScalesLogger>();
            }
            mpScalesLogger->logWeight(mpScales->weight());
        }

        if(!mpScalesFilter) {
            Msg(FILELINE, 2) << "Creating frame source";
            mpScalesFilter = std::make_unique<ScalesFilter>(
                        mpCapturer.get(), mpScales.get());
        }
        pFrameSource = mpScalesFilter.get();
    }

    Frame frame = pFrameSource->getFrame();

    Msg(FILELINE, 3) << "Allocating GStreamer frame buffer";
    GstBuffer_Handle hBuffer = gst_buffer_new_allocate(
                NULL, frame.dataSize(), NULL);
    if(!hBuffer) {
        Msg(FILELINE) << "Could not allocate GStreamer frame buffer";
        return;
    }

    Msg(FILELINE, 3) << "Setting up GStreamer frame buffer";
    Fps fps = Params()->fps;
    GST_BUFFER_PTS((GstBuffer *)hBuffer) = mTimestamp;
    GST_BUFFER_DURATION((GstBuffer *)hBuffer) =
            gst_util_uint64_scale_int(fps.den, GST_SECOND, fps.num);
    mTimestamp += GST_BUFFER_DURATION((GstBuffer *)hBuffer);

    Msg(FILELINE, 3) << "Mapping GStreamer frame buffer";
    GstMapInfo map;
    if(!gst_buffer_map(hBuffer, &map, GST_MAP_WRITE)) {
        Msg(FILELINE) << "Could not map GStreamer frame buffer";
        return;
    }
    ScopeGuard mapGuard([&hBuffer, &map](){
        gst_buffer_unmap(hBuffer, &map);
    });

    Msg(FILELINE, 3) << "Copying data into GStreamer frame buffer";
    memcpy(map.data, frame.pPixels, frame.dataSize());
    mapGuard.release();

    GstFlowReturn ret = gst_app_src_push_buffer(pAppSrc, hBuffer);
    if(ret != GST_FLOW_OK) {
        Msg(FILELINE) << "Could not push GStreamer frame buffer, error " << ret;
        return;
    }
    hBuffer.reset();

    Msg(FILELINE, 3) << "Data request for new frame finished";
}
