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

#define AGP_VIA_SEL             0xfd
#define AGP_VIA_GART_CTRL       0x80
#define AGP_VIA_APER_SIZE       0x84
#define AGP_VIA_GATT_BASE       0x88


/* NON-PUBLIC METHODS */
VOID METHOD(VIABridgeDevice, Hidd_AGPBridgeDevice, FlushGattTable)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    OOP_Object * bridgedev = gbddata->bridge->PciDevice;
    ULONG ctrlreg;
    ctrlreg = readconfiglong(bridgedev, AGP_VIA_GART_CTRL);
    ctrlreg |= (1<<7);
    writeconfiglong(bridgedev, AGP_VIA_GART_CTRL, ctrlreg);
    ctrlreg &= ~(1<<7);
    writeconfiglong(bridgedev, AGP_VIA_GART_CTRL, ctrlreg);

}

/* PUBLIC METHODS */
OOP_Object * METHOD(VIABridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID VIABridgeDevice__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg msg)
{
    /* NO OP */

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(VIABridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    struct pHidd_AGPBridgeDevice_ScanAndDetectDevices saddmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_ScanAndDetectDevices)
    };

    ULONG major;
    D(ULONG minor;)
    OOP_Object * bridgedev = NULL;
    UBYTE bridgeagpcap = 0;
    UBYTE aperture_size_value = 0;

    
    /* Scan for bridge and agp devices */
    if (!OOP_DoMethod(o, (OOP_Msg)&saddmsg))
        return FALSE;

    /* Check if bridge is a supported VIA bridge */
    if (gbddata->bridge->VendorID != 0x1106)
        return FALSE;
    
    bridgedev = gbddata->bridge->PciDevice;
    bridgeagpcap = gbddata->bridge->AgpCapability;

    /* Getting version info */ 
    major = (readconfigbyte(bridgedev, bridgeagpcap + AGP_VERSION_REG) >> 4) & 0xf;
    D(minor = readconfigbyte(bridgedev, bridgeagpcap + AGP_VERSION_REG) & 0xf);
    
    D(bug("[AGP] [VIA] Read config: AGP version %d.%d\n", major, minor));
        
    /* Getting mode */
    gbddata->bridgemode = readconfiglong(bridgedev, bridgeagpcap + AGP_STATUS_REG);
    
    D(bug("[AGP] [VIA] Reading mode: 0x%x\n", gbddata->bridgemode));

    if (major >= 3)
    {
        /* VIA AGP3 chipsets emulate pre 3.0 if in legacy mode */
        
        UBYTE reg = 0;
        reg = readconfigbyte(bridgedev, AGP_VIA_SEL);
        if ((reg & (1<<1)) == 0)
        {
            D(bug("[AGP] [VIA] 3.0 chipset working in 3.0 mode\n"));
            return FALSE;
        }
        else
        {
            D(bug("[AGP] [VIA] 3.0 chipset working in 2.0 emulation mode\n"));
        }
    }


    /* Initialize */
    
    
    /* Getting GART size */
    aperture_size_value = readconfigbyte(bridgedev, AGP_VIA_APER_SIZE);
    D(bug("[AGP] [VIA] Reading aperture size value: %x\n", aperture_size_value));
    
    switch(aperture_size_value)
    {
        case(255): gbddata->bridgeapersize = 1; break;
        case(254): gbddata->bridgeapersize = 2; break;
        case(252): gbddata->bridgeapersize = 4; break;
        case(248): gbddata->bridgeapersize = 8; break;
        case(240): gbddata->bridgeapersize = 16; break;
        case(224): gbddata->bridgeapersize = 32; break;
        case(192): gbddata->bridgeapersize = 64; break;
        case(128): gbddata->bridgeapersize = 128; break;
        case(0): gbddata->bridgeapersize = 256; break;
        default: gbddata->bridgeapersize = 0; break;
    }
    
    D(bug("[AGP] [VIA] Calculated aperture size: %d MB\n", (ULONG)gbddata->bridgeapersize));

    /* Creation of GATT table */
    struct pHidd_AGPBridgeDevice_CreateGattTable cgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_CreateGattTable)
    };
    if (!OOP_DoMethod(o, (OOP_Msg)&cgtmsg))
        return FALSE;
    
    /* Getting GART base */
    gbddata->bridgeaperbase = (IPTR)readconfiglong(bridgedev, AGP_APER_BASE);
    gbddata->bridgeaperbase &= (~0x0fUL) /* PCI_BASE_ADDRESS_MEM_MASK */;
    D(bug("[AGP] [VIA] Reading aperture base: 0x%x\n", (ULONG)gbddata->bridgeaperbase));

    /* GART control register */
    writeconfiglong(bridgedev, AGP_VIA_GART_CTRL, 0x0000000f);
        
    /* Set GATT pointer */
    writeconfiglong(bridgedev, AGP_VIA_GATT_BASE, 
        (((ULONG)(IPTR)gbddata->gatttable) & 0xfffff000) | 3);
    D(bug("[AGP] [VIA] Set GATT pointer to 0x%x\n", 
        (((ULONG)(IPTR)gbddata->gatttable) & 0xfffff000) | 3));

    gbddata->state = STATE_INITIALIZED;

    return TRUE;
}
