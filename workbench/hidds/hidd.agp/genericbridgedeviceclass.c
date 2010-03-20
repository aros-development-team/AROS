/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/agp.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "agp_private.h"

#undef HiddAGPBridgeDeviceAttrBase
#define HiddAGPBridgeDeviceAttrBase (SD(cl)->hiddAGPBridgeDeviceAB)

OOP_Object * METHOD(GenericBridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID METHOD(GenericBridgeDevice, Root, Get)
{
    ULONG idx;

    if (IS_AGPBRIDGEDEV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_AGPBridgeDevice_Mode:
                *msg->storage = 0; /* FIXME */
                return;

            case aoHidd_AGPBridgeDevice_ApertureBase:
                *msg->storage = 0; /* FIXME */
                return;

            case aoHidd_AGPBridgeDevice_ApertureSize:
                *msg->storage = 0; /* FIXME */
                return;
        }
    }
    
    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, Enable)
{
    return TRUE;
}


