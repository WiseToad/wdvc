#ifndef BUFFER_H
#define BUFFER_H

#include <cstddef>
#include <cstdint>
#include <algorithm>

    //-- template struct BufferRef --//

template <typename T>
struct BufferRef
{
    BufferRef() = default;
    BufferRef(T * pData, size_t count):
        pData(pData), count(count) {}

    size_t size() const {
        return count * sizeof(T);
    }
    size_t avail(size_t desired) const {
        return std::min(desired, count);
    }

    BufferRef operator + (size_t offset) const {
        return {pData + offset,
                count - offset};
    }
    BufferRef & operator += (size_t offset) {
        pData += offset;
        count -= offset;
        return *this;
    }
    BufferRef operator ++ (int) {
        return {pData++,
                count--};
    }
    BufferRef & operator ++ () {
        ++pData;
        --count;
        return * this;
    }

    T * pData;
    size_t count;
};

using ByteBufferRef = BufferRef<uint8_t>;

    //-- template class Buffer --//

template <typename T>
class Buffer final
{
public:
    Buffer(size_t count = 0):
        mpData(count > 0 ? new T[count] : nullptr),
        mCount(count) {}

    Buffer(Buffer && other):
        mpData(other.mpData), mCount(other.mCount)
    {
        other.reset();
    }
    ~Buffer() {
        assign(nullptr);
    }
    void operator = (Buffer && other) {
        assign(other.mpData, other.mCount);
        other.reset();
    }

    // deleted
    Buffer(const Buffer &) = delete;
    Buffer & operator = (const Buffer &) = delete;

    operator bool() const {
        return mpData;
    }
    operator T * () {
        return mpData;
    }
    operator const T * () const {
        return mpData;
    }
    T * pData() {
        return mpData;
    }
    const T * pData() const {
        return mpData;
    }
    size_t count() const {
        return mCount;
    }
    size_t size() const {
        return mCount * sizeof(T);
    }
    operator BufferRef<T> () {
        return {mpData, mCount};;
    }
    operator const BufferRef<T> () const {
        return {mpData, mCount};
    }
    BufferRef<T> ref() {
        return {mpData, mCount};
    }
    const BufferRef<T> ref() const {
        return {mpData, mCount};
    }
    Buffer dup() const {
        Buffer dup(mCount);
        memcpy(dup.mpData, mpData, size());
        return dup;
    }

private:
    void assign(T * pData) {
        delete[] mpData;
        mpData = pData;
    }
    void assign(T * pData, size_t count) {
        assign(pData);
        mCount = count;
    }
    void reset() {
        mpData = nullptr;
    }

    T * mpData;
    size_t mCount;
};

using ByteBuffer = Buffer<uint8_t>;

#endif // BUFFER_H
