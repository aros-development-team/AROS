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
        struct pHidd_AGPBridgeDevice_Initialize imsg = {
        mID : OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_Initialize)
        };
        
        OOP_Object * bridgedevice;
        /* TODO: Implement iteration over known classes and selection of device */

        bridgedevice = OOP_NewObject(NULL, CLID_Hidd_SiSAgp3BridgeDevice, NULL);
        
        if ((BOOL)OOP_DoMethod(bridgedevice, (OOP_Msg)&imsg))
            SD(cl)->bridgedevice = bridgedevice;
        else
            OOP_DisposeObject(bridgedevice);

    }

    return SD(cl)->bridgedevice;
}

