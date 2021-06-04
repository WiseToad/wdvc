#include "Common.h"
#include "Msg.h"
#include <shlwapi.h>

Text<TCHAR> getExeFullName()
{
    TCHAR exeFullNameBuf[MAX_PATH];
    if(!GetModuleFileName(NULL, exeFullNameBuf, sizeof(exeFullNameBuf))) {
        Msg(FILELINE) << "GetModuleFileName failed, error " << GetLastError();
        return {};
    }
    return exeFullNameBuf;
}

Text<TCHAR> getExePath()
{
    Text<TCHAR> exeFullName = getExeFullName();
    if(!exeFullName.length())
        return {};

    TCHAR exePathBuf[MAX_PATH];
    memcpy(exePathBuf, exeFullName, exeFullName.size());
    if(PathRemoveFileSpec(exePathBuf))
        return Text<TCHAR>(exePathBuf) + '\\';
    return exePathBuf;
}

Text<TCHAR> getTempPath()
{
    TCHAR tempPath[MAX_PATH];
    if(!GetTempPath(MAX_PATH, tempPath)) {
        Msg(FILELINE) << "GetTempPath failed, error " << GetLastError();
        return {};
    }
    return tempPath;
}

Text<TCHAR> getLogFullName()
{
    return getTempPath() + "wdvc.log";
}
