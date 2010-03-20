/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/agp.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "agp_private.h"


OOP_Object * METHOD(AGP, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

OOP_Object * METHOD(AGP, Hidd_AGP, GetBridgeDevice)
{
    /* Find bridge device matching hardware */
    if (!SD(cl)->bridgedevice)
    {
        /* TEMP!!!! */
        SD(cl)->bridgedevice = OOP_NewObject(NULL, CLID_Hidd_GenericBridgeDevice, NULL);
        /* TODO: Implement */
    }

    return SD(cl)->bridgedevice;
}

