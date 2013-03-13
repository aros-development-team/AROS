/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: LinuxFB hidd initialization code.
    Lang: English.
*/

#define DEBUG 0

#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <utility/utility.h>
#include <oop/oop.h>
#include <hidd/graphics.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <hidd/unixio.h>

#include LC_LIBDEFS_FILE

#include "linuxfb_intern.h"

static CONST_STRPTR const abd[] =
{
    IID_Hidd_Gfx,
    IID_Hidd_BitMap,
    IID_Hidd_Sync,
    IID_Hidd_PixFmt,
    IID_Hidd_ChunkyBM,
    IID_Hidd_LinuxFB,
    IID_Hidd_LinuxFBBitmap,
    NULL
};

static int Init_Hidd(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->lsd.unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!LIBBASE->lsd.unixio)
        return FALSE;

    InitSemaphore(&LIBBASE->lsd.sema);

    return !OOP_ObtainAttrBasesArray(&LIBBASE->lsd.gfxAttrBase, abd);
}

static int Expunge_Hidd(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBasesArray(&LIBBASE->lsd.gfxAttrBase, abd);

    if (LIBBASE->lsd.unixio)
        OOP_DisposeObject(LIBBASE->lsd.unixio);

    return TRUE;
}

ADD2INITLIB(Init_Hidd, 1)
ADD2EXPUNGELIB(Expunge_Hidd, 1)
