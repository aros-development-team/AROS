/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/gallium.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "gallium_intern.h"

static int HiddGallium_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.galliumAttrBase)
        OOP_ReleaseAttrBase((STRPTR)IID_Hidd_Gallium);

    return TRUE;
}

static int HiddGallium_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.galliumAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_Gallium);

    if (LIBBASE->sd.galliumAttrBase)
        return TRUE;

    return FALSE;
}

ADD2INITLIB(HiddGallium_InitLib, 0)
ADD2EXPUNGELIB(HiddGallium_ExpungeLib, 0)
