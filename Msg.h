#ifndef MSG_H
#define MSG_H

#include <iostream>
#include <sstream>
#include <memory>
#include <chrono>
#include <stdexcept>

#define MSG_SOURCE_FILE_PREFIX "../wdvc/"
#define MSG_SYNCHRONIZED
//#define MSG_USE_COPYFMT

#ifdef MSG_SYNCHRONIZED
#include <mutex>
/* Qt 5.6.3's GCC 4.9.2 doesn't need it:
#ifdef __MINGW32__
#include <mingw.mutex.h>
#endif
/**/
#endif

    //-- class FmtGuard --//

template <typename CharT>
class FmtGuard final
{
public:
    FmtGuard(std::basic_ios<CharT> & stream):
        mStream(stream),
#ifdef MSG_USE_COPYFMT
        mFmtHolder(nullptr)
#else
        mFlags(stream.flags()),
        mPrecision(stream.precision()),
        mWidth(stream.width()),
        mFill(stream.fill())
#endif
    {
#ifdef MSG_USE_COPYFMT
        mFmtHolder.copyfmt(mStream);
#endif
    }

    ~FmtGuard() {
#ifdef MSG_USE_COPYFMT
        mStream.copyfmt(mFmtHolder);
#else
        mStream.flags(mFlags);
        mStream.precision(mPrecision);
        mStream.width(mWidth);
        mStream.fill(mFill);
#endif
    }

private:
    std::basic_ios<CharT> & mStream;
#ifdef MSG_USE_COPYFMT
    std::basic_ios<CharT> mFmtHolder;
#endif
    std::ios::fmtflags mFlags;
    std::streamsize mPrecision;
    std::streamsize mWidth;
    CharT mFill;
};

    //-- class MsgTimestamp --//

class MsgTimestamp
{
public:
    MsgTimestamp():
        mTimestamp(time(nullptr)) {}

private:
    time_t mTimestamp;

    friend std::ostream & operator << (std::ostream & stream, const MsgTimestamp & timestamp) {
        char timestampStr[25];
        strftime(timestampStr, sizeof(timestampStr),
                 "%d.%m.%Y %H:%M:%S", localtime(&timestamp.mTimestamp));
        stream << timestampStr;
        return stream;
    }
};

    //-- class MsgFileline --//

#define FILELINE __FILE__, __LINE__

class MsgFileline
{
public:
    MsgFileline(const char * file, int line):
        file(removePrefix(file)), line(line) {}

private:
    const char * removePrefix(const char * file) {
        const char * prefix = MSG_SOURCE_FILE_PREFIX;
        for(const char * p = prefix; *file == *p; ++file, ++p);
        return file;
    }

    const char * file;
    int line;

    friend std::ostream & operator << (std::ostream & stream, const MsgFileline & fileline) {
        stream << fileline.file << " (" << fileline.line << ")";
        return stream;
    }
};

    //-- class Msg --//

class Msg final
{
public:
    Msg(int level = 1):
        mpGuards(mSettings.level && level <= mSettings.level ?
                      std::make_unique<Guards>() : nullptr) {
        if(mpGuards && mSettings.timestamps)
            std::cerr << MsgTimestamp() << ": ";
    }

    Msg(const char * file, int line, int level = 1):
        Msg(level) {
        if(mpGuards && mSettings.filelines)
            std::cerr << MsgFileline(file, line) << ": ";
    }

    Msg(Msg && other):
        mpGuards(std::move(other.mpGuards)) {}

    ~Msg() {
        if(mpGuards)
            std::cerr << std::endl;
    }

    // deleted
    Msg(const Msg &) = delete;
    Msg & operator = (const Msg &) = delete;

    template <typename T>
    Msg & operator << (const T & msg) {
        if(mpGuards)
            std::cerr << msg;
        return *this;
    }

    Msg & operator << (std::ostream &(* func)(std::ostream &)) {
        if(mpGuards)
            func(std::cerr);
        return *this;
    }

    static void setLevel(int level) {
        mSettings.level = std::max(level, 0);
    }

    static void setTimestamps(bool timestamps = true) {
        mSettings.timestamps = timestamps;
    }

    static void setFilelines(bool filelines = true) {
        mSettings.filelines = filelines;
    }

private:
    struct Settings {
        int level;
        bool timestamps;
        bool filelines;
    };

    class Guards final
    {
    public:
        Guards():
#ifdef MSG_SYNCHRONIZED
            mLockGuard(mutex()),
#endif
            mFmtGuard(std::cerr) {}
    private:
#ifdef MSG_SYNCHRONIZED
        std::mutex & mutex() {
            static std::mutex mutex;
            return mutex;
        }
        std::lock_guard<std::mutex> mLockGuard;
#endif
        FmtGuard<char> mFmtGuard;
    };

    static Settings mSettings;
    std::unique_ptr<Guards> mpGuards;

    friend class Err;
};

    //-- class Err --//

class Err final: public std::exception
{
public:
    Err() = default;

    Err(const char * file, int line) {
        if(Msg::mSettings.timestamps)
            mMsgSink << MsgTimestamp() << ": ";
        if(Msg::mSettings.filelines)
            mMsgSink << MsgFileline(file, line) << ": ";
    }

    Err(const Err & other):
        mMsgSink(other.mMsgSink.str()) {}

    Err(Err && other):
        /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
        mMsgSink(std::move(other.mMsgSink)) {}
        /**/
        mMsgSink(other.mMsgSink.str()) {
        other.mMsgSink.str("");
        /**/
    }

    template<class T>
    Err & operator << (const T & msg) {
        mMsgSink << msg;
        return *this;
    }

    Err & operator << (std::ostream &(* func)(std::ostream &)) {
        func(mMsgSink);
        return *this;
    }

    virtual const char * what() const noexcept override {
        mMsg = mMsgSink.str();
        return mMsg.data();
    }

private:
    std::ostringstream mMsgSink;
    mutable std::string mMsg;
};

    //-- class MsgPrefixer --//

class MsgPrefixer
{
public:
    MsgPrefixer(MsgPrefixer * pParent = nullptr):
        mpParent(pParent) {}
    virtual ~MsgPrefixer() = default;

protected:
    virtual void msgPrefix(std::ostream & stream) {}

    ::Msg Msg(int level = 1) {
        return std::move(::Msg(level) << MsgPrefix{this});
    }

    ::Msg Msg(const char * file, int line, int level = 1) {
        return std::move(::Msg(file, line, level) << MsgPrefix{this});
    }

    ::Err Err() {
        return std::move(::Err() << MsgPrefix{this});
    }

    ::Err Err(const char * file, int line) {
        return std::move(::Err(file, line) << MsgPrefix{this});
    }

private:
    struct MsgPrefix {
        MsgPrefixer * pPrefixer;
    };

    friend std::ostream & operator << (std::ostream & stream, const MsgPrefix & prefix) {
        MsgPrefixer * pPrefixer = prefix.pPrefixer;
        MsgPrefixer * pParent = pPrefixer->mpParent;
        if(pParent)
            stream << MsgPrefix{pParent};
        pPrefixer->msgPrefix(stream);
        return stream;
    }

    MsgPrefixer * mpParent;
};

#endif // MSG_H
