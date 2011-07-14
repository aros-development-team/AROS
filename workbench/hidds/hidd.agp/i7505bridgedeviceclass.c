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
#define AGP_INTEL_APER_SIZE     0xb4
#define AGP_INTEL_GATT_BASE     0xb8

#define AGP_INTEL_I7505_MCHCFG  0x50

#define IS_7505_BRIDGE(x)               \
(                                       \
    (x == 0x2550) || /* 7505_0 */       \
    (x == 0x255d)    /* 7205_0 */       \
)                                       \

/* PUBLIC METHODS */
OOP_Object * METHOD(i7505BridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}


BOOL METHOD(i7505BridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    struct pHidd_AGPBridgeDevice_ScanAndDetectDevices saddmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_ScanAndDetectDevices)
    };

    D(ULONG major, minor;)
    OOP_Object * bridgedev = NULL;
    UBYTE bridgeagpcap = 0;
    UBYTE aperture_size_value = 0;
    UWORD mchcfg = 0;    

    /* Scan for bridge and agp devices */
    if (!OOP_DoMethod(o, (OOP_Msg)&saddmsg))
        return FALSE;

    /* Check if bridge is a supported Intel bridge */
    if (gbddata->bridge->VendorID != 0x8086)
        return FALSE;
    if (!IS_7505_BRIDGE(gbddata->bridge->ProductID))
        return FALSE;
    
    bridgedev = gbddata->bridge->PciDevice;
    bridgeagpcap = gbddata->bridge->AgpCapability;

    /* Getting version info */ 
    D(major = (readconfigbyte(bridgedev, bridgeagpcap + AGP_VERSION_REG) >> 4) & 0xf);
    D(minor = readconfigbyte(bridgedev, bridgeagpcap + AGP_VERSION_REG) & 0xf);
    
    D(bug("[AGP] [Intel 7505] Read config: AGP version %d.%d\n", major, minor));
        
    /* Getting mode */
    gbddata->bridgemode = readconfiglong(bridgedev, bridgeagpcap + AGP_STATUS_REG);
    
    D(bug("[AGP] [Intel 7505] Reading mode: 0x%x\n", gbddata->bridgemode));

    gbddata->memmask = 0x00000017;


    /* Initialize */

    
    /* Getting GART size */
    aperture_size_value = readconfigbyte(bridgedev, AGP_INTEL_APER_SIZE);
    D(bug("[AGP] [Intel 7505] Reading aperture size value: %x\n", aperture_size_value));
    
    switch(aperture_size_value)
    {
        case(63): gbddata->bridgeapersize = 4; break;
        case(62): gbddata->bridgeapersize = 8; break;
        case(60): gbddata->bridgeapersize = 16; break;
        case(56): gbddata->bridgeapersize = 32; break;
        case(48): gbddata->bridgeapersize = 64; break;
        case(32): gbddata->bridgeapersize = 128; break;
        case(0): gbddata->bridgeapersize = 256; break;
        default: gbddata->bridgeapersize = 0; break;
    }
    
    D(bug("[AGP] [Intel 7505] Calculated aperture size: %d MB\n", (ULONG)gbddata->bridgeapersize));

    /* Creation of GATT table */
    struct pHidd_AGPBridgeDevice_CreateGattTable cgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_CreateGattTable)
    };
    if (!OOP_DoMethod(o, (OOP_Msg)&cgtmsg))
        return FALSE;

    
    /* Getting GART base */
    gbddata->bridgeaperbase = (IPTR)readconfiglong(bridgedev, AGP_APER_BASE);
    gbddata->bridgeaperbase &= (~0x0fUL) /* PCI_BASE_ADDRESS_MEM_MASK */;
    D(bug("[AGP] [Intel 7505] Reading aperture base: 0x%x\n", (ULONG)(IPTR)gbddata->bridgeaperbase));

    /* Set GATT pointer */
    writeconfiglong(bridgedev, AGP_INTEL_GATT_BASE, (ULONG)(IPTR)gbddata->gatttable);
    D(bug("[AGP] [Intel 7505] Set GATT pointer to 0x%x\n", (ULONG)(IPTR)gbddata->gatttable));
    
    /* Control register */
    writeconfiglong(bridgedev, AGP_INTEL_CTRL, 0x00000000);
    
    /* MCHCFG*/
    mchcfg = readconfigword(bridgedev, AGP_INTEL_I7505_MCHCFG);
    writeconfigword(bridgedev, AGP_INTEL_I7505_MCHCFG, mchcfg | (1 << 9));

    gbddata->state = STATE_INITIALIZED;

    return TRUE;    
}

