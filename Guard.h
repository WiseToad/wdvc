#ifndef GUARD_H
#define GUARD_H

#include <functional>

    //-- class ScopeGuard --//

class ScopeGuard final
{
public:
    using CloseFunc = std::function<void()>;

    ScopeGuard(const CloseFunc & closeFunc):
        mCloseFunc(closeFunc) {}

    ~ScopeGuard() {
        release();
    }

    // deleted
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard & operator = (const ScopeGuard &) = delete;

    void release() {
        if(mCloseFunc)
            mCloseFunc();
        reset();
    }
    void reset() {
        mCloseFunc = nullptr;
    }

private:
    CloseFunc mCloseFunc;
};

    //-- class ValueGuard --//

template <typename T>
class ValueGuard final
{
public:
    ValueGuard(T * pVar, T newValue):
        mpVar(pVar), mValue(*pVar) {
        *pVar = newValue;
    }
    ~ValueGuard() {
        release();
    }

    // deleted
    ValueGuard(const ValueGuard &) = delete;
    ValueGuard & operator = (const ValueGuard &) = delete;

    void release() {
        if(mpVar)
            *mpVar = mValue;
        reset();
    }
    void reset() {
        mpVar = nullptr;
    }

private:
    T * mpVar;
    T mValue;
};

#endif // GUARD_H
