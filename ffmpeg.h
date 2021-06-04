#ifndef FFMPEG_H
#define FFMPEG_H

#include "Handle.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

    //-- class SwsContext_Handle --//

using SwsContext_Handle = Handle<SwsContext *>;

template <>
inline void SwsContext_Handle::close()
{
    sws_freeContext(mHandle);
}

    //-- class AvImage --//

class AvImage final
{
public:
    AvImage():
        mPlaneCount(0), mpPlanes{}, mStrides{} {}

    ~AvImage() {
        release();
    }

    // deleted
    AvImage(const AvImage &) = delete;
    AvImage & operator = (const AvImage &) = delete;

    size_t alloc(int width, int height,  AVPixelFormat format) {
        release();
        size_t dataSize = av_image_alloc(
                    mpPlanes, mStrides, width, height, format, 16);
        if(dataSize > 0) {
            mPlaneCount = 0;
            while(mPlaneCount < 4 && mpPlanes[mPlaneCount])
                ++mPlaneCount;
        }
        return dataSize;
    }
    void release() {
        if(mPlaneCount) {
            av_freep(mpPlanes);
            mPlaneCount = 0;
        }
    }

    operator bool () const {
        return mPlaneCount;
    }
    int planeCount() const {
        return mPlaneCount;
    }
    uint8_t * const * pPlanes() const {
        return mpPlanes;
    }
    const uint8_t * pPlanes(int index) const {
        return mpPlanes[index];
    }
    const int * strides() const {
        return mStrides;
    }
    int strides(int index) const {
        return mStrides[index];
    }

private:
    int mPlaneCount;
    uint8_t * mpPlanes[4];
    int mStrides[4];
};

#endif // FFMPEG_H
