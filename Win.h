#ifndef WIN_H
#define WIN_H

#include "Handle.h"
#include <windows.h>

    //-- class HANDLE_Handle --//

using HANDLE_Handle = Handle<HANDLE>;

template <>
inline void HANDLE_Handle::close()
{
    CloseHandle(mHandle);
}

    //-- class HKEY_Handle --//

using HKEY_Handle = Handle<HKEY>;

template <>
inline void HKEY_Handle::close()
{
    RegCloseKey(mHandle);
}

#endif // WIN_H
