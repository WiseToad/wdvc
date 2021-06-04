#ifndef TIMING_H
#define TIMING_H

#include <chrono>

    //-- struct TimePoint --//

struct TimePoint
{
    TimePoint():
        mTimePoint(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        mTimePoint = std::chrono::high_resolution_clock::now();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mTimePoint;

    friend struct TimeInterval;
};

    //-- struct TimeInterval --//

struct TimeInterval
{
    TimeInterval(const TimePoint & startPoint):
        mDuration(std::chrono::high_resolution_clock::now() - startPoint.mTimePoint) {}

    double seconds() {
        return mDuration.count();
    }

private:
    std::chrono::duration<double> mDuration;
};

    //-- struct Timeout --//

class Timeout
{
public:
    Timeout(int defaultTimeout = 0): // in milliseconds
        mDefaultTimeout(defaultTimeout), mReached(true) {}

    void start(int timeout = 0) {
        mTimeout = (timeout > 0 ? timeout : mDefaultTimeout);
        mStartTime.reset();
        mReached = false;
    }

    operator bool () {
        if(mReached)
            return true;
        TimeInterval elapsed(mStartTime);
        if(elapsed.seconds() * 1000 >= mTimeout) {
            mReached = true;
            return true;
        }
        return false;
    }

private:
    int mDefaultTimeout;
    int mTimeout;
    TimePoint mStartTime;
    bool mReached;
};

#endif // TIMING_H
