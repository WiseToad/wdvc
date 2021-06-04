#ifndef STATEFUL_H
#define STATEFUL_H

enum struct SimpleState {
    valid, invalid
};

enum struct CommonState {
    initial, valid, invalid
};

enum struct ProcessState {
    initial, running, failed, zombie
};

    //-- class Stateful --//

template <typename T, T initialState, T okState>
class Stateful
{
public:
    using State = T;

    Stateful(T state = initialState):
        mState(state) {}

    virtual ~Stateful() = default;

    T state() const {
        return mState;
    }
    bool state(T state) const {
        return (mState == state);
    }
    explicit operator bool () const {
        return state(okState);
    }

protected:
    void setState(T state) {
        mState = state;
    }

private:
    T mState;
};

using SimpleStateful = Stateful<SimpleState,
    SimpleState::valid, SimpleState::valid>;
using CommonStateful = Stateful<CommonState,
    CommonState::initial, CommonState::valid>;
using ProcessStateful = Stateful<ProcessState,
    ProcessState::initial, ProcessState::running>;

#endif // STATEFUL_H
