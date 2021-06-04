#include "Daemon.h"
#include "Msg.h"
#include "Params.h"
#include "Common.h"
#include "Buffer.h"
#include "Server.h"
#include <tlhelp32.h>
#include <shlwapi.h>
#include <io.h>

namespace {

const Text<TCHAR> cRegKeyName   = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const Text<TCHAR> cRegValueName = "wdvc";

} // namespace

    //-- class Daemon --//

HKEY_Handle Daemon::regOpenKey(HKEY hParent, const TCHAR * keyName, REGSAM access)
{
    HKEY_Handle hKey;
    LONG res = RegOpenKeyEx(hParent, keyName, 0, access, &hKey);
    if(res != ERROR_SUCCESS)
        Msg(FILELINE) << "Could not open registry key, error " << res;
    return hKey;
}

Text<TCHAR> Daemon::getCmdLine(int argc, char * argv[])
{
    Text<TCHAR> cmdLine = '"' + getExeFullName() + '"';
    for(int i = 1; i < argc; ++i)
        cmdLine += ' ' + Text<TCHAR>(argv[i]);
    return cmdLine;
}

void Daemon::install(int argc, char * argv[])
{
    Msg(FILELINE, 2) << "Opening registry key for read: \"" << cRegKeyName << "\"";
    HKEY_Handle hKey = regOpenKey(HKEY_LOCAL_MACHINE, cRegKeyName, KEY_READ);
    if(!hKey)
        return;

    Msg(FILELINE, 2) << "Querying registry value with name: " << cRegValueName;
    TCHAR regValue[4096] = {};
    DWORD regValueType, regValueSize = sizeof(regValue);
    LONG res = RegQueryValueEx(hKey, cRegValueName, NULL, &regValueType,
                              (BYTE*)regValue, &regValueSize);
    if(res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) {
        Msg(FILELINE) << "Could not query registry value, error " << res;
        return;
    }
    Msg(FILELINE, 3) << "Registry value: " << CharText(regValue);

    Msg(FILELINE, 2) << "Building command line";
    Text<TCHAR> cmdLine = getCmdLine(argc, argv);
    Msg(FILELINE, 3) << "Command line: " << cmdLine;
    if(regValue == cmdLine) {
        Msg(FILELINE) << "Autorun was installed earlier";
        return;
    }

    Msg(FILELINE, 2) << "Reopening registry key for write";
    hKey = regOpenKey(HKEY_LOCAL_MACHINE, cRegKeyName, KEY_WRITE);
    if(!hKey)
        return;

    Msg(FILELINE, 2) << "Set registry key value";
    res = RegSetValueEx(hKey, cRegValueName, 0, REG_SZ,
                        (BYTE*)cmdLine.pStr(), cmdLine.size());
    if(res != ERROR_SUCCESS) {
        Msg(FILELINE) << "Could not set registry key value, error " << res;
        return;
    }

    Msg(FILELINE) << "Autorun installed";
}

void Daemon::uninstall()
{
    Msg(FILELINE, 2) << "Opening registry key for write: \"" << cRegKeyName << "\"";
    HKEY_Handle hKey = regOpenKey(HKEY_LOCAL_MACHINE, cRegKeyName, KEY_WRITE);
    if(!hKey)
        return;

    Msg(FILELINE, 2) << "Deleting registry key value";
    LONG res = RegDeleteValue(hKey, cRegValueName);
    if(res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) {
        Msg(FILELINE) << "Could not delete registry key value, error " << res;
        return;
    }

    Msg(FILELINE) << (res == ERROR_FILE_NOT_FOUND ?
                          "Nothing to uninstall" : "Autorun uninstalled");
}

void Daemon::start(int argc, char * argv[])
{
    kill();

    Msg(FILELINE, 2) << "Building command line";
    Text<TCHAR> cmdLine = getCmdLine(argc, argv);
    Buffer<TCHAR> cmdLineBuf(cmdLine.length() + 1);
    memcpy(cmdLineBuf, cmdLine, cmdLine.size());
    Msg(FILELINE, 3) << "Command line: " << Text<TCHAR>(cmdLineBuf.pData());

    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    SECURITY_ATTRIBUTES secAttr = {};
    /**/
    SECURITY_ATTRIBUTES secAttr;
    memset(&secAttr, 0, sizeof(secAttr));
    /**/
    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.bInheritHandle = TRUE;

    Msg(FILELINE, 2) << "Opening log file for daemon";
    HANDLE hLogFile = CreateFile(
                getLogFullName(), GENERIC_WRITE, FILE_SHARE_READ,
                &secAttr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hLogFile == INVALID_HANDLE_VALUE) {
        Msg(FILELINE) << "Could not open log file for daemon, error " << GetLastError();
        return;
    }

    Msg(FILELINE, 2) << "Moving to end of daemon log file";
    if(SetFilePointer(hLogFile, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER) {
        Msg(FILELINE) << "Could not move to end of daemon log file, error " << GetLastError();
        return;
    }

    /* Qt 5.6.3's GCC 4.9.2 doesn't like it:
    STARTUPINFO startInfo = {};
    /**/
    STARTUPINFO startInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    /**/
    startInfo.cb = sizeof(STARTUPINFO);
    startInfo.dwFlags = STARTF_USESTDHANDLES;
    startInfo.hStdOutput = hLogFile;
    startInfo.hStdError = hLogFile;
    startInfo.hStdInput = INVALID_HANDLE_VALUE;

    PROCESS_INFORMATION procInfo;

    Msg(FILELINE, 2) << "Creating child process";
    if(!CreateProcess(NULL, cmdLineBuf, NULL, NULL, TRUE,
                      DETACHED_PROCESS, NULL, NULL, &startInfo, &procInfo)) {
        Msg(FILELINE) << "Could not create child process, error " << GetLastError();
        return;
    }

    Msg(FILELINE) << "Process started";
}

void Daemon::kill()
{
    Msg(FILELINE, 2) << "Obtaining EXE file name";
    Text<TCHAR> exeFullName = getExeFullName();
    Text<TCHAR> exeFileName = PathFindFileName(exeFullName);
    Msg(FILELINE, 3) << "EXE file name: " << exeFileName;

    Msg(FILELINE, 2) << "Obtaining current process PID";
    DWORD currentPid = GetCurrentProcessId();
    Msg(FILELINE, 3) << "Current process PID: " << currentPid;

    Msg(FILELINE, 2) << "Obtaining process list snapshot";
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    if(hSnapshot == INVALID_HANDLE_VALUE) {
        Msg(FILELINE) << "Could not obtain process list snapshot, error "
                      << GetLastError();
        return;
    }

    PROCESSENTRY32 procEntry;
    procEntry.dwSize = sizeof(procEntry);

    Msg(FILELINE, 2) << "Start iterating over process list";
    bool found = false;
    BOOL res = Process32First(hSnapshot, &procEntry);
    while(res) {
        if(procEntry.szExeFile == exeFileName &&
                procEntry.th32ProcessID != currentPid) {
            Msg(FILELINE, 2) << "Found PID: " << procEntry.th32ProcessID
                             << ", opening process";
            found = true;
            HANDLE_Handle hProcess = OpenProcess(
                        PROCESS_TERMINATE | SYNCHRONIZE, 0,
                        (DWORD)procEntry.th32ProcessID);
            if(!hProcess) {
                Msg(FILELINE) << "Could not open process to stop it, error "
                              << GetLastError();
            } else {
                Msg(FILELINE, 2) << "Terminating process";
                TerminateProcess(hProcess, EXIT_SUCCESS);
                if(WaitForSingleObject(hProcess, 1000) == WAIT_OBJECT_0) {
                    Msg(FILELINE) << "Process stopped";
                } else {
                    Msg(FILELINE) << "Process is about to stop";
                    Sleep(1000);
                }
            }
        }
        res = Process32Next(hSnapshot, &procEntry);
    }
    if(!found)
        Msg(FILELINE) << "Nothing to stop";
}

bool Daemon::active() const
{
    return (GetStdHandle(STD_INPUT_HANDLE) == INVALID_HANDLE_VALUE);
}
