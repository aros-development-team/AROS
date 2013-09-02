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

/* NON-PUBLIC METHODS */
VOID METHOD(Agp3BridgeDevice, Hidd_AGPBridgeDevice, FlushGattTable)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    OOP_Object * bridgedev = gbddata->bridge->PciDevice;
    UBYTE bridgeagpcap = gbddata->bridge->AgpCapability;
    ULONG ctrlreg;
    ctrlreg = readconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG);
    writeconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG, ctrlreg & ~AGP_CTRL_REG_GTBLEN);
    writeconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG, ctrlreg);
}

/* PUBLIC METHODS */
OOP_Object * METHOD(Agp3BridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID Agp3BridgeDevice__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg msg)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);
    
    if (gbddata->state != STATE_UNKNOWN)
    {
        OOP_Object * bridgedev = gbddata->bridge->PciDevice;
        UBYTE bridgeagpcap = gbddata->bridge->AgpCapability;
        ULONG ctrlreg;
        ctrlreg = readconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG);
        writeconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG, ctrlreg & ~AGP_CTRL_REG_APEREN);
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(Agp3BridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);    

    /* Subclass is required to perform the detection */
    if (!gbddata->bridge)
        return FALSE;

    OOP_Object * bridgedev = gbddata->bridge->PciDevice;
    UBYTE bridgeagpcap = gbddata->bridge->AgpCapability;
    UWORD aperture_size_value = 0;
    ULONG ctrlreg = 0;
    
    /* Getting GART size */
    aperture_size_value = readconfigword(bridgedev, bridgeagpcap + AGP_APER_SIZE_REG);
    D(bug("[AGP] Reading aperture size value: %x\n", aperture_size_value));
    
    switch(aperture_size_value)
    {
        case(0xf3f): gbddata->bridgeapersize = 4; break;
        case(0xf3e): gbddata->bridgeapersize = 8; break;
        case(0xf3c): gbddata->bridgeapersize = 16; break;
        case(0xf38): gbddata->bridgeapersize = 32; break;
        case(0xf30): gbddata->bridgeapersize = 64; break;
        case(0xf20): gbddata->bridgeapersize = 128; break;
        case(0xf00): gbddata->bridgeapersize = 256; break;
        case(0xe00): gbddata->bridgeapersize = 512; break;
        case(0xc00): gbddata->bridgeapersize = 1024; break;
        case(0x800): gbddata->bridgeapersize = 2048; break;
        default: gbddata->bridgeapersize = 0; break;
    }
    
    D(bug("[AGP] Calculated aperture size: %d MB\n", (ULONG)gbddata->bridgeapersize));

    /* Creation of GATT table */
    struct pHidd_AGPBridgeDevice_CreateGattTable cgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_CreateGattTable)
    };
    if (!OOP_DoMethod(o, (OOP_Msg)&cgtmsg))
        return FALSE;
    
    /* Getting GART base */
    gbddata->bridgeaperbase = (IPTR)readconfiglong(bridgedev, AGP_APER_BASE);
    gbddata->bridgeaperbase &= (~0x0fUL) /* PCI_BASE_ADDRESS_MEM_MASK */;
    D(bug("[AGP] Reading aperture base: 0x%x\n", (ULONG)gbddata->bridgeaperbase));

    /* Set GATT pointer */
    writeconfiglong(bridgedev, bridgeagpcap + AGP_GATT_CTRL_LO_REG,
        (ULONG)(IPTR)gbddata->gatttable);
    D(bug("[AGP] Set GATT pointer to 0x%x\n", (ULONG)gbddata->gatttable));
    
    /* Enabled GART and GATT */
    ctrlreg = readconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG);
    writeconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG,
        ctrlreg | AGP_CTRL_REG_APEREN | AGP_CTRL_REG_GTBLEN);
    
    D(bug("[AGP] Enabled GART and GATT\n"));
    
    return TRUE;
}

