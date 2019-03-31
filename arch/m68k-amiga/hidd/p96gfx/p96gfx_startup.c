/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "p96gfx_intern.h"
#include "p96gfx_bitmap.h"

#include LC_LIBDEFS_FILE

BOOL P96GFX__Initialise(LIBBASETYPEPTR LIBBASE);

#undef SysBase
#undef OOPBase

static int P96GFX_LibInit(LIBBASETYPEPTR LIBBASE)
{
    struct ExecBase *SysBase = LIBBASE->csd.cs_SysBase;
    struct Library  *OOPBase = LIBBASE->csd.cs_OOPBase;
    LIBBASE->csd.cs_GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    struct Library *GfxBase = LIBBASE->csd.cs_GfxBase;

    D(bug("[HiddP96Gfx] %s() ******************************\n", __func__));

    if (!GfxBase)
        return FALSE;

    LIBBASE->csd.basebm = OOP_FindClass(CLID_Hidd_BitMap);

    if (!P96GFX__Initialise(LIBBASE)) {
        D(bug("[HiddP96Gfx] %s: P96GFX__Initialise failed\n", __func__));
        CloseLibrary(GfxBase);
        return FALSE;
    }

    return TRUE;
}
ADD2INITLIB(P96GFX_LibInit, 0)
