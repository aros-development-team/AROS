/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/gallium.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "softpipe_intern.h"

static int HiddSoftpipe_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.UtilityBase)
        CloseLibrary(LIBBASE->sd.UtilityBase);

    if (LIBBASE->sd.CyberGfxBase)
        CloseLibrary(LIBBASE->sd.CyberGfxBase);

    if (LIBBASE->sd.hiddGalliumAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    return TRUE;
}

static int HiddSoftpipe_InitLib(LIBBASETYPEPTR LIBBASE)
{
    if ((LIBBASE->sd.UtilityBase = OpenLibrary((STRPTR)"utility.library",0)))
    {
        if ((LIBBASE->sd.CyberGfxBase = OpenLibrary((STRPTR)"cybergraphics.library",0)))
        {
            if ((LIBBASE->sd.hiddGalliumAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium)))
                return TRUE;
            CloseLibrary(LIBBASE->sd.CyberGfxBase);
        }
        CloseLibrary(LIBBASE->sd.UtilityBase);
    }
    return FALSE;
}

ADD2INITLIB(HiddSoftpipe_InitLib, 0)
ADD2EXPUNGELIB(HiddSoftpipe_ExpungeLib, 0)

ADD2LIBS((STRPTR)"gallium.hidd", 7, static struct Library *, GalliumHiddBase);
