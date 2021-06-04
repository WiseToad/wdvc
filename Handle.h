#ifndef HANDLE_H
#define HANDLE_H

#include "Ptr.h"

    //-- template class Handle --//

template <typename T>
class Handle final
{
public:
    Handle(T handle = {}):
        mHandle(handle) {}

    Handle(Handle && other):
        mHandle(other.mHandle)
    {
        other.reset();
    }
    ~Handle() {
        release();
    }
    void operator = (Handle && other) {
        assign(other.mHandle);
        other.reset();
    }

    // deleted
    Handle(const Handle &) = delete;
    Handle & operator = (const Handle &) = delete;

    operator bool() const  {
        return mHandle;
    }
    operator T () {
        return mHandle;
    }
    PtrType<T> operator -> () {
        return ptr(mHandle);
    }
    T handle() {
        return mHandle;
    }
    T * operator & () {
        release();
        return &mHandle;
    }
    Handle dup() {
        addRef();
        return Handle(mHandle);
    }
    void release() {
        assign({});
    }
    void reset() {
        mHandle = {};
    }

private:
    void assign(T handle) {
        if(*this) // operator bool
            close();
        mHandle = handle;
    }

    // need specialization
    void addRef();
    void close();

    T mHandle;
};

#endif // HANDLE_H
