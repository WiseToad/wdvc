#ifndef DIRECTX_H
#define DIRECTX_H

#include "Handle.h"
#include <d3d9.h>

    //-- class IDirect3D9_Handle --//

using IDirect3D9_Handle = Handle<IDirect3D9 *>;

template <>
inline void IDirect3D9_Handle::close()
{
    mHandle->Release();
}

    //-- class IDirect3DDevice9_Handle --//

using IDirect3DDevice9_Handle = Handle<IDirect3DDevice9 *>;

template <>
inline void IDirect3DDevice9_Handle::close()
{
    mHandle->Release();
}

    //-- class IDirect3DSurface9_Handle --//

using IDirect3DSurface9_Handle = Handle<IDirect3DSurface9 *>;

template <>
inline void IDirect3DSurface9_Handle::close()
{
    mHandle->Release();
}

#endif // DIRECTX_H
