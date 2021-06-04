#include "Frame.h"

    //-- struct FrameSize --//

FrameSize FrameSize::scaled(const FrameScale & scale) const
{
    if(scale.num > 0 || scale.den > 0)
        return {width  * scale.num / scale.den,
                height * scale.num / scale.den};
    if(scale.width > 0 || scale.height > 0)
        return {scale.width, scale.height};
    return *this;
}

FrameSize FrameSize::aligned(unsigned align) const
{
    align--;
    align |= (align >> 1);
    align |= (align >> 2);
    align |= (align >> 4);
    align |= (align >> 8);
    align |= (align >> 16);

    return {width & ~align, height & ~align};
}

FrameSize FrameSize::bounded(const FrameSize & minSize, const FrameSize & maxSize) const
{
    if(width < minSize.width || height < minSize.height)
        return minSize;
    if(width > maxSize.width || height > maxSize.height)
        return maxSize;
    return *this;
}
