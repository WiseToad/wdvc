#ifndef PARAMS_H
#define PARAMS_H

#include "Capturer.h"
#include "Frame.h"

    //-- enum struct Option --//

enum struct Option: int {
    start, remove, logfile, console, help
};

    //-- class Params --//

class Params final
{
public:
    class Impl final
    {
    public:
        void showUsage() const;

        Option option;
        CapturerType capturerType;
        Fps fps;
        FrameScale scale;    // frame scaling params
        const char * preset; // x264 preset
        unsigned bitrate;    // in kbps
        unsigned crf;        // Constant Rate Factor (see http://slhck.info/video/2017/02/24/crf-guide.html)
        unsigned crfMax;     // worst CRF as caused by VBV (Video Buffering Verifier)
        unsigned keyint;     // keyframe frequency (every keyint-th frame will be keyframe)
        bool intraRefresh;   // forbid IDR frames
        unsigned rtspPort;
        unsigned comPort;
        FramePos panelPos;
        std::string dbHost;
        unsigned dbPort;
        std::string dbUser;
        std::string dbPass;
        bool traceSource;
        int traceLevel;
        int gstTraceLevel;

    private:
        void init(int argc, char * argv[]);
        void parseSwitch(int switchIdx, char * pSwitchParam);
        void throwUsage() const;

        friend class Params;
    };

    const Impl * operator -> () const {
        return pImpl();
    }

private:
    static void init(int argc, char * argv[]) {
        pImpl()->init(argc, argv);
    }
    static Impl * pImpl();

    friend int main(int, char *[]);
};

#endif // PARAMS_H
