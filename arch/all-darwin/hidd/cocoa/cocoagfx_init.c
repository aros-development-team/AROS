/*
    Copyright (C) 2024, The AROS Development Team. All rights reserved.

    Desc: Cocoa GFX HIDD initialization
*/

#define DEBUG 0
#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <oop/oop.h>
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

#include "cocoa_intern.h"

static CONST_STRPTR const abd[] =
{
    IID_Hidd_Gfx,
    IID_Hidd_BitMap,
    IID_Hidd_Sync,
    IID_Hidd_PixFmt,
    IID_Hidd_ColorMap,
    IID_Hidd_ChunkyBM,
    NULL
};

static int CocoaGfx_Init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[CocoaGfx] Init\n"));

    InitSemaphore(&LIBBASE->csd.sema);

    return !OOP_ObtainAttrBasesArray(&LIBBASE->csd.gfxAttrBase, abd);
}

static int CocoaGfx_Expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[CocoaGfx] Expunge\n"));

    OOP_ReleaseAttrBasesArray(&LIBBASE->csd.gfxAttrBase, abd);

    return TRUE;
}

ADD2INITLIB(CocoaGfx_Init, 1)
ADD2EXPUNGELIB(CocoaGfx_Expunge, 1)
