#ifndef X264_H
#define X264_H

#include "Handle.h"

extern "C" {
#include <../include/x264.h>
}

    //-- class x264_Handle --//

using x264_Handle = Handle<x264_t *>;

template <>
inline void x264_Handle::close()
{
    x264_encoder_close(mHandle);
}

#endif // X264_H
