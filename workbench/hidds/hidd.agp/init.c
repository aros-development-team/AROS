/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <hidd/agp.h>
#include <proto/oop.h>
#include <proto/exec.h>

#include "agp_private.h"

static int AgpHidd_ExpungeLib(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->sd.bridgedevice)
        OOP_DisposeObject(LIBBASE->sd.bridgedevice);

    return TRUE;
}

static int AgpHidd_InitLib(LIBBASETYPEPTR LIBBASE)
{
    LIBBASE->sd.bridgedevice = NULL;

    LIBBASE->sd.hiddAGPBridgeDeviceAB = OOP_ObtainAttrBase((STRPTR)IID_Hidd_AGPBridgeDevice);

    if (LIBBASE->sd.hiddAGPBridgeDeviceAB)
        return TRUE;

    return TRUE;
}

ADD2INITLIB(AgpHidd_InitLib, 0)
ADD2EXPUNGELIB(AgpHidd_ExpungeLib, 0)

