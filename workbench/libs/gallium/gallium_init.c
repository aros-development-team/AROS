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

    return TRUE;
}

static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->driver)
        OOP_DisposeObject(LIBBASE->driver);

    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
