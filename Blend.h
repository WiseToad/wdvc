#ifndef BLEND_H
#define BLEND_H

#include "Frame.h"

extern "C" {

void blendImage(Pixel * pTargetPixels, int targetPitch, int width, int height,
                Pixel * pSourcePixels, int sourcePitch);

}

#endif // BLEND_H
