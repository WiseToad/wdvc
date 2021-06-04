#ifndef SYSHANDLER_H
#define SYSHANDLER_H

    //-- class SysHandler --//

class SysHandler final
{
public:
    static void init();

    // made static - quick and dirty
    static bool isIntRequest() {
        return mIsIntRequest;
    }

private:
    SysHandler();
    ~SysHandler();

    static void signalHandler(int signal);
    static void terminateHandler();

    void (* mpPrevSigsegv)(int);
    void (* mpPrevSigfpe)(int);
    void (* mpPrevSigill)(int);
    void (* mpPrevSigint)(int);
    void (* mpPrevTerm)(void);

    // made static - quick and dirty
    static bool mIsIntRequest;
};

#endif // SYSHANDLER_H
