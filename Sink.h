#ifndef SINK_H
#define SINK_H

template <typename SinkT>
using SinkFuncPtr = void (SinkT::*)(uint8_t *, size_t);

    //-- class Sink --//

class Sink
{
public:
    virtual ~Sink() = default;

    virtual void consume(uint8_t *, size_t) {}
};

    //-- template class SinkProxy --//

template <typename SinkT>
class SinkProxy: public Sink
{
public:
    SinkProxy(SinkT * pSink, SinkFuncPtr<SinkT> pSinkFunc):
        mpSink(pSink), mpSinkFunc(pSinkFunc) {}

    virtual void consume(uint8_t * pData, size_t size) override {
        if(mpSink && pData && size)
            (mpSink->*mpSinkFunc)(pData, size);
    }

private:
    SinkT * mpSink;
    SinkFuncPtr<SinkT> mpSinkFunc;
};

#endif // SINK_H
