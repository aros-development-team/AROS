/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/gallium.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "softpipe_intern.h"

static int SoftpipeHidd_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.SoftpipeCyberGfxBase)
        CloseLibrary(LIBBASE->sd.SoftpipeCyberGfxBase);

    if (LIBBASE->sd.hiddGalliumAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    return TRUE;
}

static int SoftpipeHidd_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.SoftpipeCyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library",0);

    LIBBASE->sd.hiddGalliumAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);

    if (LIBBASE->sd.SoftpipeCyberGfxBase && LIBBASE->sd.hiddGalliumAB)
        return TRUE;

    return FALSE;
}

ADD2INITLIB(SoftpipeHidd_InitLib, 0)
ADD2EXPUNGELIB(SoftpipeHidd_ExpungeLib, 0)

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
