/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/agp.h>
#include <proto/oop.h>
#include <proto/exec.h>
#define DEBUG 0
#include <aros/debug.h>

#include "agp_private.h"

#undef HiddAGPBridgeDeviceAttrBase
#define HiddAGPBridgeDeviceAttrBase (SD(cl)->hiddAGPBridgeDeviceAB)

#define AGP_INTEL_NBXCFG        0x50
#define AGP_INTEL_CTRL          0xb0

/* NON-PUBLIC METHODS */
VOID METHOD(i8XXBridgeDevice, Hidd_AGPBridgeDevice, FlushGattTable)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    OOP_Object * bridgedev = gbddata->bridge->PciDevice;
    ULONG ctrlreg;
    
    ctrlreg = readconfiglong(bridgedev, AGP_INTEL_CTRL);
    writeconfiglong(bridgedev, AGP_INTEL_CTRL, ctrlreg & ~(1 << 7));
    ctrlreg = readconfiglong(bridgedev, AGP_INTEL_CTRL);
    writeconfiglong(bridgedev, AGP_INTEL_CTRL, ctrlreg | (1 << 7));
}

/* PUBLIC METHODS */
OOP_Object * METHOD(i8XXBridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID i8XXBridgeDevice__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg msg)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);
    
    if (gbddata->state != STATE_UNKNOWN)
    {
        OOP_Object * bridgedev = gbddata->bridge->PciDevice;
        UWORD cfgreg;
        
        cfgreg = readconfigword(bridgedev, AGP_INTEL_NBXCFG);
        writeconfigword(bridgedev, AGP_INTEL_NBXCFG, cfgreg & ~(1 << 9));
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(i8XXBridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    bug("[i8XXBridgeDevice] abstract Initialize called\n");
    return FALSE;
}
