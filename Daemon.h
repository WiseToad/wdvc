#ifndef DAEMON_H
#define DAEMON_H

#include "Win.h"
#include "Buffer.h"
#include "Text.h"

    //-- class Daemon --//

class Daemon final
{
public:
    void install(int argc, char * argv[]);
    void uninstall();

    void start(int argc, char * argv[]);
    void kill();

    bool active() const;

private:
    HKEY_Handle regOpenKey(HKEY hParent, const TCHAR * keyName, REGSAM access);
    Text<TCHAR> getCmdLine(int argc, char * argv[]);
};

#endif // DAEMON_H
