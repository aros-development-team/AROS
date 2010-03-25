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

OOP_Object * METHOD(SiSAgp3BridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

BOOL METHOD(SiSAgp3BridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    struct pHidd_AGPBridgeDevice_ScanAndDetectDevices saddmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_ScanAndDetectDevices)
    };

    ULONG major, minor = 0;
    OOP_Object * bridgedev = NULL;
    UBYTE bridgeagpcap = 0;
    
    /* Scan for bridge and agp devices */
    if (!OOP_DoMethod(o, (OOP_Msg)&saddmsg))
        return FALSE;

    /* Check if bridge is a supported SiS bridge */
    if (gbddata->bridge->VendorID != 0x1039)
        return FALSE;
    
    /* Check if version is at least 3.5 */
    bridgedev = gbddata->bridge->PciDevice;
    bridgeagpcap = gbddata->bridge->AgpCapability;

    /* Getting version info */ 
    major = (readconfigbyte(bridgedev, bridgeagpcap + AGP_VERSION_REG) >> 4) & 0xf;
    minor = readconfigbyte(bridgedev, bridgeagpcap + AGP_VERSION_REG) & 0xf;
    D(bug("[AGP] [SiS] Read config: AGP version %d.%d\n", major, minor));

    /* In case of SiS only 3.5 bridges are guaranteed to be AGP3 compliant */
    if (!((major == 3) && (minor >= 5)))
        return FALSE;
    
    /* Getting mode */
    gbddata->bridgemode = readconfiglong(bridgedev, bridgeagpcap + AGP_STATUS_REG);
    D(bug("[AGP] [SiS] Reading mode: 0x%x\n", gbddata->bridgemode));

    /* Execute standard AGP3 initialize */
    if (OOP_DoSuperMethod(cl, o, (OOP_Msg) msg))
    {
        gbddata->state = STATE_INITIALIZED;
        return TRUE;
    }
    else
        return FALSE;
}
