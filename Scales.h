#ifndef SCALES_H
#define SCALES_H

#include "Stateful.h"
#include "Timing.h"
#include "Win.h"
#include "Buffer.h"
#include "Frame.h"
#include "MySQL.h"
#include "Timing.h"
#include <deque>
#include <thread>
#include <mutex>
#include <vector>

/* Qt 5.6.3's GCC 4.9.2 doesn't need it:
#ifdef __MINGW32__
#include <mingw.thread.h>
#include <mingw.mutex.h>
#endif
/**/

    //-- class Scales --//

class Scales: public CommonStateful
{
public:
    enum struct WeightState: int {
        unknown, stable, unstable, overload
    };
    struct Weight
    {
        Weight():
            state(WeightState::unknown), value(0.0) {}

        WeightState state;
        double value;
    };

    Scales();

    // deleted
    Scales(const Scales &) = delete;
    Scales & operator = (const Scales &) = delete;

    void updateWeight();

    Weight weight() const;

private:
    void init();
    void update();
    void updateStat(DWORD comStatEvent);
    void updateData(const ByteBufferRef & data);
    void parseLine(const std::string & line);
    std::vector<std::string> splitLine(const std::string & line, char delim);
    std::string trimToken(const std::string & token);
    void parseWeight(const std::string & token);
    void recover();
    void reset();

    Timeout mRecoveryTimeout;
    Timeout mIdleTimeout;
    HANDLE_Handle mhComPort;
    HANDLE_Handle mhStatEvent;
    OVERLAPPED mStatOverlapped;
    bool mStatWait;
    HANDLE_Handle mhReadEvent;
    OVERLAPPED mReadOverlapped;
    bool mReadWait;
    ByteBuffer mReadBuf;
    std::string mReadPending;
    Weight mWeight;
};

    //-- class ScalesFilter --//

class ScalesFilter final: public FrameSource
{
public:
    ScalesFilter(FrameSource * pSource, Scales * pScales);

    virtual Frame getFrame() override;

private:
    FrameSource * mpSource;
    Scales * mpScales;
};

    //-- class ScalesLogger --//

class ScalesLogger final
{
public:
    ScalesLogger();
    ~ScalesLogger();

    ScalesLogger(const ScalesLogger &) = delete;
    ScalesLogger & operator = (const ScalesLogger &) = delete;

    void logWeight(const Scales::Weight & weight);

private:
    struct LogEntry {
        Scales::Weight weight;
        time_t timestamp;
    };

    void writerMain();
    bool writeEntry(const LogEntry & entry);

    std::deque<LogEntry> mLog;
    Scales::Weight mPriorWeight;
    std::mutex mLogMutex;
    MYSQL_Handle mhCon;
    bool mStopWriter;
    std::thread mWriterThread;
    bool mWriterConnected;
    bool mWriterFailed;
};

#endif // SCALES_H
