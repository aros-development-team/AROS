/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/gallium.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "gallium_intern.h"

static int GalliumHidd_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.driver)
        OOP_DisposeObject(LIBBASE->sd.driver);

    if (LIBBASE->sd.loadedDriverHidd)
        CloseLibrary(LIBBASE->sd.loadedDriverHidd);

    if (LIBBASE->sd.hiddGalliumAB)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    return TRUE;
}

static int GalliumHidd_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.loadedDriverHidd = NULL;
    LIBBASE->sd.driver = NULL;

    LIBBASE->sd.hiddGalliumAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);

    if (LIBBASE->sd.hiddGalliumAB)
        return TRUE;

    return FALSE;
}

ADD2INITLIB(GalliumHidd_InitLib, 0)
ADD2EXPUNGELIB(GalliumHidd_ExpungeLib, 0)

