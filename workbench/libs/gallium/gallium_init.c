/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE
#include "gallium_intern.h"


static int Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&LIBBASE->driversemaphore);
    LIBBASE->driver = NULL;
    LIBBASE->drivermodule = NULL;
    LIBBASE->galliumAttrBase = OOP_ObtainAttrBase(IID_Hidd_Gallium);

    if (!LIBBASE->galliumAttrBase)
        return FALSE;

    return TRUE;
}

static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->driver)
        OOP_DisposeObject(LIBBASE->driver);

    if (LIBBASE->galliumAttrBase)
        OOP_ReleaseAttrBase(IID_Hidd_Gallium);

    if (LIBBASE->drivermodule)
        CloseLibrary(LIBBASE->drivermodule);

    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);

ADD2LIBS((STRPTR)"gallium.hidd", 0, static struct Library *, GalliumHiddBase);
