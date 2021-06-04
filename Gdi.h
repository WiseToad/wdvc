#ifndef GDI_H
#define GDI_H

#include "Handle.h"
#include <windows.h>

    //-- class WndHDC_Handle --//

struct WndHDC
{
    WndHDC() = default;

    WndHDC(HWND hWnd):
        hWnd(hWnd), hDC(GetDC(hWnd)) {}

    HWND hWnd;
    HDC hDC;
};

using WndHDC_Handle = Handle<WndHDC>;

template <>
inline WndHDC_Handle::operator bool () const
{
    return mHandle.hDC;
}

template <>
inline void WndHDC_Handle::close()
{
    ReleaseDC(mHandle.hWnd, mHandle.hDC);
}

    //-- class HDC_Handle --//

using HDC_Handle = Handle<HDC>;

template <>
inline void HDC_Handle::close()
{
    DeleteDC(mHandle);
}

    //-- class HDC_State --//

class HDC_State final
{
public:
    HDC_State(HDC hDC = HDC(-1)):
        mhDC(hDC != HDC(-1) && SaveDC(hDC) ? hDC : HDC(-1)) {}

    ~HDC_State() {
        restore();
    }

    // deleted
    HDC_State(const HDC_State &) = delete;
    HDC_State & operator = (const HDC_State &) = delete;

    void save(HDC hDC) {
        restore();
        if(hDC != HDC(-1) && SaveDC(hDC))
            mhDC = hDC;
    }
    void restore() {
        if(mhDC != HDC(-1))
            RestoreDC(mhDC, -1);
        mhDC = HDC(-1);
    }

private:
    HDC mhDC;
};

    //-- class HBITMAP_Handle --//

using HBITMAP_Handle = Handle<HBITMAP>;

template <>
inline void HBITMAP_Handle::close()
{
    DeleteObject(mHandle);
}

    //-- class HBRUSH_Handle --//

using HBRUSH_Handle = Handle<HBRUSH>;

template <>
inline void HBRUSH_Handle::close()
{
    DeleteObject(mHandle);
}

    //-- class HFONT_Handle --//

using HFONT_Handle = Handle<HFONT>;

template <>
inline void HFONT_Handle::close()
{
    DeleteObject(mHandle);
}

    //-- class HGDIOBJ_Guard --//

class HGDIOBJ_Guard
{
public:
    HGDIOBJ_Guard(HDC hDC, HGDIOBJ hObj):
        mhDC(hDC), mhObj(hObj) {}

    HGDIOBJ_Guard(HGDIOBJ_Guard && other):
        mhDC(other.mhDC), mhObj(other.mhObj) {
        other.reset();
    }

    ~HGDIOBJ_Guard() {
        release();
    }

    // deleted
    HGDIOBJ_Guard(const HGDIOBJ_Guard &) = delete;
    HGDIOBJ_Guard & operator = (const HGDIOBJ_Guard &) = delete;

    operator bool() {
        return mhDC && mhObj;
    }
    void release() {
        if(mhDC && mhObj)
            SelectObject(mhDC, mhObj);
        reset();
    }
    void reset() {
        mhDC = NULL;
        mhObj = NULL;
    }

private:
    HDC mhDC;
    HGDIOBJ mhObj;
};

inline HGDIOBJ_Guard SelectObjectGuarded(HDC hDC, HGDIOBJ hObj)
{
    return HGDIOBJ_Guard(hDC, SelectObject(hDC, hObj));
}

#endif // GDI_H
