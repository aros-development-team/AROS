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

#define AGP_SIS_GATT_BASE   0x90
#define AGP_SIS_APER_SIZE   0x94
#define AGP_SIS_GATT_CNTRL  0x97
#define AGP_SIS_GATT_FLUSH  0x98

/* NON-PUBLIC METHODS */
VOID METHOD(SiSBridgeDevice, Hidd_AGPBridgeDevice, FlushGattTable)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    OOP_Object * bridgedev = gbddata->bridge->PciDevice;
    writeconfigbyte(bridgedev, AGP_SIS_GATT_FLUSH, 0x02);
}

/* PUBLIC METHODS */
OOP_Object * METHOD(SiSBridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    return o;
}

VOID SiSBridgeDevice__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg msg)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);
    
    if (gbddata->state != STATE_UNKNOWN)
    {
        OOP_Object * bridgedev = gbddata->bridge->PciDevice;
        UBYTE temp = 0;

        /* Disable GART */
        temp = readconfigbyte(bridgedev, AGP_SIS_APER_SIZE);
        writeconfigbyte(bridgedev, AGP_SIS_APER_SIZE, temp & ~3);
        
        /* Disable GATT */
        writeconfigbyte(bridgedev, AGP_SIS_GATT_CNTRL, 0x00);
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(SiSBridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    struct HIDDGenericBridgeDeviceData * gbddata =
        OOP_INST_DATA(SD(cl)->genericBridgeDeviceClass, o);

    struct pHidd_AGPBridgeDevice_ScanAndDetectDevices saddmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_ScanAndDetectDevices)
    };

    ULONG major, minor = 0;
    OOP_Object * bridgedev = NULL;
    UBYTE bridgeagpcap = 0;
    UBYTE aperture_size_value = 0;
    UBYTE temp = 0;

    
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
    if (((major == 3) && (minor >= 5)))
        return FALSE;
    
    /* Getting mode */
    gbddata->bridgemode = readconfiglong(bridgedev, bridgeagpcap + AGP_STATUS_REG);
    D(bug("[AGP] [SiS] Reading mode: 0x%x\n", gbddata->bridgemode));



    /* Initialize */
    
    /* Getting GART size */
    aperture_size_value = readconfigbyte(bridgedev, AGP_SIS_APER_SIZE);
    D(bug("[AGP] [SIS] Reading aperture size value: %x\n", aperture_size_value));
    
    switch(aperture_size_value & ~(0x07))
    {
        case( 3 & ~(0x07)): gbddata->bridgeapersize = 4; break;
        case(19 & ~(0x07)): gbddata->bridgeapersize = 8; break;
        case(35 & ~(0x07)): gbddata->bridgeapersize = 16; break;
        case(51 & ~(0x07)): gbddata->bridgeapersize = 32; break;
        case(67 & ~(0x07)): gbddata->bridgeapersize = 64; break;
        case(83 & ~(0x07)): gbddata->bridgeapersize = 128; break;
        case(99 & ~(0x07)): gbddata->bridgeapersize = 256; break;
        default: gbddata->bridgeapersize = 0; break;
    }
    
    D(bug("[AGP] [SIS] Calculated aperture size: %d MB\n", (ULONG)gbddata->bridgeapersize));

    /* Creation of GATT table */
    struct pHidd_AGPBridgeDevice_CreateGattTable cgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_CreateGattTable)
    };
    if (!OOP_DoMethod(o, (OOP_Msg)&cgtmsg))
        return FALSE;
    
    /* Getting GART base */
    gbddata->bridgeaperbase = (IPTR)readconfiglong(bridgedev, AGP_APER_BASE);
    gbddata->bridgeaperbase &= (~0x0fUL) /* PCI_BASE_ADDRESS_MEM_MASK */;
    D(bug("[AGP] [SiS] Reading aperture base: 0x%x\n", (ULONG)gbddata->bridgeaperbase));
    
    /* Set GATT pointer */
    writeconfiglong(bridgedev, AGP_SIS_GATT_BASE, (ULONG)(IPTR)gbddata->gatttable);
    D(bug("[AGP] [SiS] Set GATT pointer to 0x%x\n", (ULONG)(IPTR)gbddata->gatttable));
    
    /* Enable GART */
    temp = readconfigbyte(bridgedev, AGP_SIS_APER_SIZE);
    writeconfigbyte(bridgedev, AGP_SIS_APER_SIZE, temp | 3);

    /* Enable GATT */
    writeconfigbyte(bridgedev, AGP_SIS_GATT_CNTRL, 0x05);

    gbddata->state = STATE_INITIALIZED;

    return TRUE;
}
