#ifndef MYSQL_H
#define MYSQL_H

#include "Handle.h"
#include <windows.h>
#include <../include/mysql.h>

    //-- class MySQL_Handle --//

using MYSQL_Handle = Handle<MYSQL *>;

template<>
inline void MYSQL_Handle::close()
{
    mysql_close(mHandle);
}

#endif // MYSQL_H
