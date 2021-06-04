#ifndef GSTREAMER_H
#define GSTREAMER_H

#include "Handle.h"
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

    //-- class GMainLoop_Handle --//

using GMainLoop_Handle = Handle<GMainLoop *>;

template <>
inline void GMainLoop_Handle::close()
{
    g_main_loop_unref(mHandle);
}

    //-- class GstElement_Handle --//

using GstElement_Handle = Handle<GstElement *>;

template <>
inline void GstElement_Handle::addRef()
{
    gst_object_ref(GST_OBJECT(mHandle));
}

template <>
inline void GstElement_Handle::close()
{
    gst_object_unref(GST_OBJECT(mHandle));
}

    //-- class GstBuffer_Handle --//

using GstBuffer_Handle = Handle<GstBuffer *>;

template <>
inline void GstBuffer_Handle::close()
{
    gst_buffer_unref(mHandle);
}

    //-- class GstRTSPMountPoints_Handle --//

using GstRTSPMountPoints_Handle = Handle<GstRTSPMountPoints *>;

template <>
inline void GstRTSPMountPoints_Handle::close()
{
    g_object_unref(mHandle);
}

#endif // GSTREAMER_H
