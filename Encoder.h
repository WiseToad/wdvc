#ifndef ENCODER_H
#define ENCODER_H

#include "Frame.h"
#include "Sink.h"
#include "x264.h"
#include "ffmpeg.h"
#include "Timing.h"
#include <memory>

    //-- class Encoder --//

class Encoder
{
public:
    virtual ~Encoder() = default;

    virtual void encode(const Frame & rgbFrame) = 0;
    virtual void flush() {}

protected:
    template <typename SinkT>
    Encoder(SinkT * pSink, SinkFuncPtr<SinkT> pSinkFunc):
        mpSink(std::make_unique<SinkProxy<SinkT>>(pSink, pSinkFunc)) {}

    void deliver(unsigned char * pData, size_t size) {
        mpSink->consume(pData, size);
    }

private:
    std::unique_ptr<Sink> mpSink;
};

    //-- class H264Encoder --//

class H264Encoder final: public Encoder
{
public:
    enum struct NalMode {
        wholeBulk, withStartcodes, withoutStartcodes
    };

    template <typename SinkT>
    H264Encoder(SinkT * pSink, SinkFuncPtr<SinkT> pSinkFunc,
                NalMode nalMode = NalMode::wholeBulk):
        Encoder(pSink, pSinkFunc), mRecoveryTimeout(3000),
        mNalMode(nalMode), mFrameCount(0) {}

    ~H264Encoder();

    // deleted
    H264Encoder(const & H264Encoder) = delete;
    H264Encoder & operator = (const & H264Encoder) = delete;

    virtual void encode(const Frame & frame) override;
    virtual void flush() override;

private:
    void encode(x264_picture_t * picture);

    Timeout mRecoveryTimeout;
    NalMode mNalMode;
    FrameSize mEncodeSize;
    SwsContext_Handle mhConvertCtx;
    AvImage mYuvImage;
    x264_Handle mhEncoder;
    int mFrameCount;
};

#endif // ENCODER_H
