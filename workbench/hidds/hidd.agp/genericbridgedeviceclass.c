/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/agp.h>
#include <hidd/pci.h>
#include <proto/oop.h>
#include <proto/exec.h>
#define DEBUG 1
#include <aros/debug.h>

#include "agp_private.h"

#undef HiddAGPBridgeDeviceAttrBase
#define HiddAGPBridgeDeviceAttrBase (SD(cl)->hiddAGPBridgeDeviceAB)

struct HiddAgpPciDevicesEnumeratorData
{
    struct HIDDGenericBridgeDeviceData  *gbddata;
    OOP_AttrBase                        hiddPCIDeviceAB;
};

/* HELPERS */
UBYTE readconfigbyte(OOP_Object * pciDevice, UBYTE where)
{
    struct pHidd_PCIDevice_ReadConfigByte rcbmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte),
    reg: where,
    }, *msg = &rcbmsg;
    
    return (UBYTE)OOP_DoMethod(pciDevice, (OOP_Msg)msg);
}

UWORD readconfigword(OOP_Object * pciDevice, UBYTE where)
{
    struct pHidd_PCIDevice_ReadConfigWord rcwmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord),
    reg: where,
    }, *msg = &rcwmsg;
    
    return (UWORD)OOP_DoMethod(pciDevice, (OOP_Msg)msg);    
}

ULONG readconfiglong(OOP_Object * pciDevice, UBYTE where)
{
    struct pHidd_PCIDevice_ReadConfigLong rclmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong),
    reg: where,
    }, *msg = &rclmsg;
    
    return (ULONG)OOP_DoMethod(pciDevice, (OOP_Msg)msg);
}

VOID writeconfiglong(OOP_Object * pciDevice, UBYTE where, ULONG val)
{
    struct pHidd_PCIDevice_WriteConfigLong wclmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong),
    reg: where,
    val: val,
    }, *msg = &wclmsg;
    
    OOP_DoMethod(pciDevice, (OOP_Msg)msg); 
}

VOID writeconfigbyte(OOP_Object * pciDevice, UBYTE where, UBYTE val)
{
    struct pHidd_PCIDevice_WriteConfigByte wcbmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte),
    reg: where,
    val: val,
    }, *msg = &wcbmsg;
    
    OOP_DoMethod(pciDevice, (OOP_Msg)msg); 
}

VOID writeconfigword(OOP_Object * pciDevice, UBYTE where, UWORD val)
{
    struct pHidd_PCIDevice_WriteConfigWord wcwmsg = {
    mID: OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord),
    reg: where,
    val: val,
    }, *msg = &wcwmsg;
    
    OOP_DoMethod(pciDevice, (OOP_Msg)msg); 
}

AROS_UFH3(void, HiddAgpPciDevicesEnumerator,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, pciDevice, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    IPTR class, agpcapptr;
    struct HiddAgpPciDevicesEnumeratorData * hdata = 
                        (struct HiddAgpPciDevicesEnumeratorData *)hook->h_Data;

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase hdata->hiddPCIDeviceAB

    OOP_GetAttr(pciDevice, aHidd_PCIDevice_Class, &class);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_CapabilityAGP, (APTR)&agpcapptr);
    
    /* Select bridges and AGP devices */
    if ((class == 0x06) || (agpcapptr))
    {
        IPTR intline;
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &intline);

        /* If the device is not a bridge and it has not interrupt line set, skip it */
        if ((class != 0x06) && ((intline == 0) || (intline >= 255)))
            return;

        struct PciAgpDevice * pciagpdev = 
            (struct PciAgpDevice *)AllocVec(sizeof(struct PciAgpDevice), MEMF_ANY | MEMF_CLEAR);
        IPTR temp;
        pciagpdev->PciDevice = pciDevice;
        pciagpdev->AgpCapability = (UBYTE)agpcapptr;
        pciagpdev->Class = (UBYTE)class;
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &temp);
        pciagpdev->VendorID = (UWORD)temp;
        OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &temp);
        pciagpdev->ProductID = (UWORD)temp;
        
        AddTail(&hdata->gbddata->devices, (struct Node *)pciagpdev);
    }

#undef HiddPCIDeviceAttrBase

    AROS_USERFUNC_EXIT
}

/* NON-PUBLIC METHODS */
BOOL METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, ScanAndDetectDevices)
{
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);
    struct PciAgpDevice * pciagpdev = NULL;

    if (!SD(cl)->pcibus)
        return FALSE;
        
    /* Scan all PCI devices */
    struct HiddAgpPciDevicesEnumeratorData hdata = {
    gbddata :           gbddata,
    hiddPCIDeviceAB :   SD(cl)->hiddPCIDeviceAB
    };
    
    struct Hook FindHook = {
    h_Entry:    (IPTR (*)())HiddAgpPciDevicesEnumerator,
    h_Data:     &hdata,
    };

    struct TagItem Requirements[] = {
    { TAG_DONE,             0UL }
    };

    struct pHidd_PCI_EnumDevices enummsg = {
    mID:        OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
    callback:   &FindHook,
    requirements:   (struct TagItem*)&Requirements,
    };

    OOP_DoMethod(SD(cl)->pcibus, (OOP_Msg)&enummsg);

    /* Select matching devices */
    ForeachNode(&gbddata->devices, pciagpdev)
    {
        /* Select bridge */
        if ((!gbddata->bridge) && (pciagpdev->Class == 0x06) && 
            (pciagpdev->AgpCapability))
        {
            gbddata->bridge = pciagpdev;
        }

        /* Select video card */
        if ((!gbddata->videocard) && (pciagpdev->Class == 0x03) &&
            (pciagpdev->AgpCapability))
        {
            gbddata->videocard = pciagpdev;
        }
    }
    
    return (gbddata->videocard && gbddata->bridge);
}

BOOL METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, CreateGattTable)
{
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);
    
    if (gbddata->bridgeapersize == 0)
        return FALSE;

    /* Create a table that will hold a certain number of 32bit pointers */
    ULONG entries = gbddata->bridgeapersize * 1024 * 1024 / 4096;
    ULONG tablesize =  entries * 4;
    ULONG i = 0;
    
    gbddata->scratchmembuffer = AllocVec(4096 * 2, MEMF_PUBLIC | MEMF_CLEAR);
    gbddata->scratchmem = (ULONG*)(ALIGN((IPTR)gbddata->scratchmembuffer, 4096));
    D(bug("[AGP] Created scratch memory at 0x%x\n", (ULONG)gbddata->scratchmem));

    
    gbddata->gatttablebuffer = AllocVec(tablesize + 4096, MEMF_PUBLIC | MEMF_CLEAR);
    gbddata->gatttable = (ULONG *)(ALIGN((ULONG)gbddata->gatttablebuffer, 4096));
    
    D(bug("[AGP] Created GATT table size %d at 0x%x\n", tablesize, 
        (ULONG)gbddata->gatttable));
    
    for (i = 0; i < entries; i++)
    {
        writel((ULONG)gbddata->scratchmem, gbddata->gatttable + i);
        readl(gbddata->gatttable + i);	/* PCI Posting. */
    }
    
//FIXME    flush_cpu_cache();
    return TRUE;
}

/* PUBLIC METHODS */
OOP_Object * METHOD(GenericBridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);

    if (o)
    {
        struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);
        gbddata->bridge = NULL;
        gbddata->bridgemode = 0;
        gbddata->bridgeapersize = 0;
        gbddata->bridgeaperbase = 0;
        gbddata->gatttable = NULL;
        gbddata->gatttablebuffer = NULL;
        gbddata->scratchmem = NULL;
        gbddata->scratchmembuffer = NULL;
        gbddata->videocard = NULL;
        NEWLIST(&gbddata->devices);
    }

    return o;
}

VOID GenericBridgeDevice__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg msg)
{
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);
    struct PciAgpDevice * pciagpdev = NULL;

    /* TODO: Deinitialize AGP (or maybe in subclasses only)? */

    /* Free scanned device information */
    while((pciagpdev = (struct PciAgpDevice *)RemHead(&gbddata->devices)) != NULL)
        FreeVec(pciagpdev);

    /* Free GATT table */
    D(bug("[AGP] Freeing GATT table 0x%x, scratch memory 0x%x\n",
        (ULONG)gbddata->gatttable, (ULONG)gbddata->scratchmem));

    if (gbddata->scratchmembuffer)
        FreeVec(gbddata->scratchmembuffer);
    gbddata->scratchmembuffer = NULL;
    gbddata->scratchmem = NULL;
    
    if (gbddata->gatttablebuffer)
        FreeVec(gbddata->gatttablebuffer);
    gbddata->gatttablebuffer = NULL;
    gbddata->gatttable = NULL;

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(GenericBridgeDevice, Root, Get)
{
    ULONG idx;
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);

    if (IS_AGPBRIDGEDEV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_AGPBridgeDevice_Mode:
                *msg->storage = gbddata->bridgemode;
                return;

            case aoHidd_AGPBridgeDevice_ApertureBase:
                *msg->storage = gbddata->bridgeaperbase;
                return;

            case aoHidd_AGPBridgeDevice_ApertureSize:
                *msg->storage = gbddata->bridgeapersize;
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

BOOL METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    bug("[GenericBridgeDevice] abstract initialize method called\n");
    return FALSE;
}
