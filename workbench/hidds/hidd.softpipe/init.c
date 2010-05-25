/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/gallium.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "softpipe.h"

static int SoftpipeHidd_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.SoftpipeCyberGfxBase)
        CloseLibrary(LIBBASE->sd.SoftpipeCyberGfxBase);

    if (LIBBASE->sd.hiddGalliumBaseDriverAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_GalliumBaseDriver);

    return TRUE;
}

static int SoftpipeHidd_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.SoftpipeCyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library",0);

    LIBBASE->sd.hiddGalliumBaseDriverAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_GalliumBaseDriver);

    if (LIBBASE->sd.SoftpipeCyberGfxBase && LIBBASE->sd.hiddGalliumBaseDriverAB)
        return TRUE;

    return FALSE;
}

ADD2INITLIB(SoftpipeHidd_InitLib, 0)
ADD2EXPUNGELIB(SoftpipeHidd_ExpungeLib, 0)

