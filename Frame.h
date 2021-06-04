#ifndef FRAME_H
#define FRAME_H

#include <cstddef>
#include <cstdint>

    //-- struct Pixel --//

struct Pixel final
{
    uint8_t B, G, R, A;
};

inline Pixel * addPitch(Pixel * pPixels, size_t pitch)
{
    return reinterpret_cast<Pixel *>(reinterpret_cast<uint8_t *>(pPixels) + pitch);
}

    //-- struct FrameScale --//

struct FrameScale final
{
    unsigned num, den;
    size_t width, height;
};

    //-- struct FrameSize --//

struct FrameSize final
{
    bool operator == (const FrameSize size) const {
        return (size.width == width && size.height == height);
    }
    bool operator != (const FrameSize size) const {
        return (size.width != width || size.height != height);
    }
    size_t area() const {
        return (width * height);
    }

    FrameSize scaled(const FrameScale & scale) const;
    FrameSize aligned(unsigned align) const;
    FrameSize bounded(const FrameSize & minSize, const FrameSize & maxSize) const;

    size_t width, height;
};

    //-- struct FramePos --//

struct FramePos final
{
    int x, y;
};

    //-- struct Frame --//

struct Frame final
{
    Frame() = default;

    Frame(const FrameSize & size, Pixel * pData = nullptr):
        size(size), pitch(size.width * sizeof(Pixel)), pPixels(pData) {}

    Pixel * pLine(int index) {
        return addPitch(pPixels, pitch * index);
    }
    size_t dataSize() const {
        return pitch * size.height;
    }
    bool valid() const {
        return (size.width > 0 && size.height > 0 &&
                pitch >= size.width * sizeof(Pixel) && pPixels);
    }

    FrameSize size; // in pixels
    size_t pitch;   // in bytes
    Pixel * pPixels;
};

    //-- class FrameSource --//

class FrameSource
{
public:
    virtual ~FrameSource() = default;

    virtual Frame getFrame() = 0;
};

    //-- struct Fps --//

struct Fps final
{
    Fps() = default;
    Fps(int fps):
        num(fps), den(1) {}

    bool isValid() const {
        return (num > 0 && den > 0 &&
                num <= 50 * den && num >= den);
    }

    unsigned num, den;
};

#endif // FRAME_H
