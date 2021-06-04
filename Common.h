#ifndef COMMON_H
#define COMMON_H

#include "Text.h"
#include <windows.h>

Text<TCHAR> getExeFullName();
Text<TCHAR> getExePath();
Text<TCHAR> getTempPath();
Text<TCHAR> getLogFullName();

#endif // COMMON_H
