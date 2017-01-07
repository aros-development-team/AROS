/*
    Copyright 2010-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/gfx.h>

#include LC_LIBDEFS_FILE

CONST_STRPTR softpipe_str = "softpipe";

static int Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&LIBBASE->driversemaphore);
    LIBBASE->driver = NULL;
    LIBBASE->drivermodule = NULL;

    LIBBASE->gfxAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gfx);
    if (!LIBBASE->gfxAttrBase)
        return FALSE;

    LIBBASE->bmAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap);
    if (!LIBBASE->bmAttrBase)
        return FALSE;

    LIBBASE->galliumAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);
    if (!LIBBASE->galliumAttrBase)
        return FALSE;

    LIBBASE->fallback = (char *)softpipe_str;
    LIBBASE->fallbackmodule = NULL;

    /* cache method id's that we use ..  */
    LIBBASE->galliumMId_UpdateRect = OOP_GetMethodID(IID_Hidd_BitMap, moHidd_BitMap_UpdateRect);
    LIBBASE->galliumMId_DisplayResource = OOP_GetMethodID(IID_Hidd_Gallium, moHidd_Gallium_DisplayResource);

    return TRUE;
}

static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->driver)
        OOP_DisposeObject(LIBBASE->driver);

    if (LIBBASE->galliumAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    if (LIBBASE->bmAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_BitMap);

    if (LIBBASE->gfxAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gfx);

    if (LIBBASE->drivermodule)
        CloseLibrary(LIBBASE->drivermodule);

    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 0, static struct Library *, GalliumHiddBase);
