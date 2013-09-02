/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/agp.h>
#include <hidd/pci.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/dos.h> /* FIXME: Remove after removing Delay() */
#define DEBUG 0
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

static ULONG HiddAgpGenericAgp3CalibrateModes(ULONG requestedmode, ULONG bridgemode, ULONG vgamode)
{
    ULONG calibratedmode = bridgemode;
    ULONG temp = 0;
    
    /* Apply reserved mask to requestedmode */
    requestedmode &= ~AGP3_RESERVED_MASK;
    
    /* Select a speed for requested mode */
    temp = requestedmode & 0x07;
    requestedmode &= ~0x07; /* Clear any speed */
    if (temp & AGP_STATUS_REG_AGP3_X8)
        requestedmode |= AGP_STATUS_REG_AGP3_X8;
    else
        requestedmode |= AGP_STATUS_REG_AGP3_X4;
    
    /* Set ARQSZ as max value. Ignore requestedmode */
    calibratedmode = ((calibratedmode & ~AGP_STATUS_REG_ARQSZ_MASK) |
        max(calibratedmode & AGP_STATUS_REG_ARQSZ_MASK, vgamode & AGP_STATUS_REG_ARQSZ_MASK));
    
    /* Set calibration cycle. Ignore requestedmode */
    calibratedmode = ((calibratedmode & ~AGP_STATUS_REG_CAL_MASK) |
        min(calibratedmode & AGP_STATUS_REG_CAL_MASK, vgamode & AGP_STATUS_REG_CAL_MASK));
    
    /* Set SBA for AGP3 (always) */
    calibratedmode |= AGP_STATUS_REG_SBA;
    
    /* Select speed based on request and capabilities of bridge and vgacard */
    calibratedmode &= ~0x07; /* Clear any mode */
    if ((requestedmode & AGP_STATUS_REG_AGP3_X8) &&
        (bridgemode & AGP_STATUS_REG_AGP3_X8) &&
        (vgamode & AGP_STATUS_REG_AGP3_X8))
        calibratedmode |= AGP_STATUS_REG_AGP3_X8;
    else
        calibratedmode |= AGP_STATUS_REG_AGP3_X4;

    return calibratedmode;
}

static ULONG HiddAgpGenericAgp2CalibrateModes(ULONG requestedmode, ULONG bridgemode, ULONG vgamode)
{
    ULONG calibratedmode = bridgemode;
    ULONG temp = 0;

    /* Apply reserved mask to requestedmode */
    requestedmode &= ~AGP2_RESERVED_MASK;
    
    /* Fix for some bridges reporting only one speed instead of all */
    if (bridgemode & AGP_STATUS_REG_AGP2_X4)
        bridgemode |= (AGP_STATUS_REG_AGP2_X2 | AGP_STATUS_REG_AGP2_X1);
    if (bridgemode & AGP_STATUS_REG_AGP2_X2)
        bridgemode |= AGP_STATUS_REG_AGP2_X1;

    /* Select speed for requested mode */
    temp = requestedmode & 0x07;
    requestedmode &= ~0x07; /* Clear any speed */
    if (temp & AGP_STATUS_REG_AGP2_X4)
        requestedmode |= AGP_STATUS_REG_AGP2_X4;
    else 
    {
        if (temp & AGP_STATUS_REG_AGP2_X2)
            requestedmode |= AGP_STATUS_REG_AGP2_X2;
        else
            requestedmode |= AGP_STATUS_REG_AGP2_X1;
    }
    
    /* Disable SBA if not supported/requested */
    if (!((bridgemode & AGP_STATUS_REG_SBA) && (requestedmode & AGP_STATUS_REG_SBA)
            && (vgamode & AGP_STATUS_REG_SBA)))
        calibratedmode &= ~AGP_STATUS_REG_SBA;

    /* Select speed based on request and capabilities of bridge and vgacard */
    calibratedmode &= ~0x07; /* Clear any mode */
    if ((requestedmode & AGP_STATUS_REG_AGP2_X4) &&
        (bridgemode & AGP_STATUS_REG_AGP2_X4) &&
        (vgamode & AGP_STATUS_REG_AGP2_X4))
        calibratedmode |= AGP_STATUS_REG_AGP2_X4;
    else
    {
        if ((requestedmode & AGP_STATUS_REG_AGP2_X2) &&
            (bridgemode & AGP_STATUS_REG_AGP2_X2) &&
            (vgamode & AGP_STATUS_REG_AGP2_X2))
            calibratedmode |= AGP_STATUS_REG_AGP2_X2;
        else
            calibratedmode |= AGP_STATUS_REG_AGP2_X1;
    }

    /* Disable fast writed if in X1 mode */
    if (calibratedmode & AGP_STATUS_REG_AGP2_X1)
        calibratedmode &= ~AGP_STATUS_REG_FAST_WRITES;
    
    return calibratedmode;
}

static ULONG HiddAgpGenericSelectBestMode(struct HIDDGenericBridgeDeviceData * gbddata, ULONG requestedmode, ULONG bridgemode)
{
    OOP_Object * videodev = gbddata->videocard->PciDevice;
    UBYTE videoagpcap = gbddata->videocard->AgpCapability;    
    ULONG vgamode = 0;

    /* Get VGA card capability */
    vgamode = readconfiglong(videodev, videoagpcap + AGP_STATUS_REG);
    
    D(bug("[AGP]     VGA mode 0x%x\n", vgamode));
    
    /* Set Request Queue */
    bridgemode = ((bridgemode & ~AGP_STATUS_REG_RQ_DEPTH_MASK) |
        min(requestedmode & AGP_STATUS_REG_RQ_DEPTH_MASK,
        min(bridgemode & AGP_STATUS_REG_RQ_DEPTH_MASK, vgamode & AGP_STATUS_REG_RQ_DEPTH_MASK)));
    
    /* Fast Writes */
    if (!(
        (bridgemode & AGP_STATUS_REG_FAST_WRITES) &&
        (requestedmode & AGP_STATUS_REG_FAST_WRITES) &&
        (vgamode & AGP_STATUS_REG_FAST_WRITES)))
    {
        bridgemode &= ~AGP_STATUS_REG_FAST_WRITES;
    }
        
    if (gbddata->bridgemode & AGP_STATUS_REG_AGP_3_0)
    {
        bridgemode = HiddAgpGenericAgp3CalibrateModes(requestedmode, bridgemode, vgamode);
    }
    else
    {
        bridgemode = HiddAgpGenericAgp2CalibrateModes(requestedmode, bridgemode, vgamode);
    }
    
    return bridgemode;
}

static VOID HiddAgpGenericSendCommand(struct HIDDGenericBridgeDeviceData * gbddata, ULONG status)
{
    struct PciAgpDevice * pciagpdev = NULL;

    /* Send command to all AGP capable devices */
    ForeachNode(&gbddata->devices, pciagpdev)
    {
        if(pciagpdev->AgpCapability)
        {
            ULONG mode = status;
            
            mode &= 0x7;
            if (status & AGP_STATUS_REG_AGP_3_0)
                mode *= 4;
            
            D(bug("[AGP] Set AGP%d device 0x%x/0x%x to speed %dx\n", 
                (status & AGP_STATUS_REG_AGP_3_0) ? 3 : 2, 
                pciagpdev->VendorID, pciagpdev->ProductID, mode));
            
            writeconfiglong(pciagpdev->PciDevice, pciagpdev->AgpCapability + AGP_COMMAND_REG, status);

            /* FIXME: Change this to timer.device interaction. DOS may not be up when agp.hidd is used */
            /* Keep this delay here. Some bridges need it. */
            Delay(10);
        }
    }
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
    gbddata->gatttable = (ULONG *)(ALIGN((IPTR)gbddata->gatttablebuffer, 4096));
    
    D(bug("[AGP] Created GATT table size %d at 0x%x\n", tablesize, 
        (ULONG)gbddata->gatttable));
    
    for (i = 0; i < entries; i++)
    {
        writel((ULONG)(IPTR)gbddata->scratchmem, gbddata->gatttable + i);
        readl(gbddata->gatttable + i);	/* PCI Posting. */
    }
    
    flushcpucache();

    return TRUE;
}

VOID METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, FlushGattTable)
{
    bug("[GenericBridgeDevice] abstract FlushGattTable method called\n");
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
        gbddata->state = STATE_UNKNOWN;
        gbddata->memmask = 0x00000000;
        InitSemaphore(&gbddata->lock);
    }

    return o;
}

VOID GenericBridgeDevice__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg msg)
{
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);
    struct PciAgpDevice * pciagpdev = NULL;

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
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);

    ObtainSemaphore(&gbddata->lock);
    
    if (gbddata->state != STATE_INITIALIZED)
    {
        ReleaseSemaphore(&gbddata->lock);
        return FALSE;
    }
    
    OOP_Object * bridgedev = gbddata->bridge->PciDevice;
    UBYTE bridgeagpcap = gbddata->bridge->AgpCapability;
    ULONG requestedmode = msg->requestedmode;
    ULONG bridgemode = 0;
    ULONG major = 0;
    
    
    D(bug("[AGP] Enable AGP:\n"));
    D(bug("[AGP]     Requested mode 0x%x\n", requestedmode));
    
    bridgemode = readconfiglong(bridgedev, bridgeagpcap + AGP_STATUS_REG);
    D(bug("[AGP]     Bridge mode 0x%x\n", requestedmode));
    
    bridgemode = HiddAgpGenericSelectBestMode(gbddata, requestedmode, bridgemode);
    
    bridgemode |= AGP_STATUS_REG_AGP_ENABLED;
    
    major = (readconfigbyte(bridgedev, bridgeagpcap + AGP_VERSION_REG) >> 4) & 0xf;

    if (major >= 3)
    {
        /* Bridge supports version 3 or greater) */
        if (gbddata->bridgemode & AGP_STATUS_REG_AGP_3_0)
        {
            /* Bridge is operating in mode 3.0 */
        }
        else
        {
            /* Bridge is operating in legacy mode */
            /* Disable calibration cycle */
            ULONG temp = 0;
            bridgemode &= ~(7 << 10);
            temp = readconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG);
            temp |= (1 << 9);
            writeconfiglong(bridgedev, bridgeagpcap + AGP_CTRL_REG, temp);           
        }
    }

    D(bug("[AGP] Mode to write: 0x%x\n", bridgemode));
    
    HiddAgpGenericSendCommand(gbddata, bridgemode);
    gbddata->state = STATE_ENABLED;
    
    ReleaseSemaphore(&gbddata->lock);
    
    return TRUE;
}

VOID METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, BindMemory)
{
    ULONG i;
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);

    D(bug("Bind address 0x%x into offset %d, size %d\n", (ULONG)msg->address, msg->offset, msg->size));

    if (gbddata->state != STATE_ENABLED)
        return;

    ObtainSemaphore(&gbddata->lock);

    /* TODO: check if offset + size / 4096 ends before gatt_table end */
    
    /* TODO: get mask type */
    
    /* TODO: Check if each entry in GATT to be written is unbound */
    
    /* Flush incomming memory - will be done in flush_cpu_cache below */
    
    /* Insert entries into GATT table */
    for(i = 0; i < msg->size / 4096; i++)
    {
        /* Write masked memory address into GATT */
        writel((msg->address + (4096 * i)) | gbddata->memmask, 
            gbddata->gatttable + msg->offset + i);
    }
    
    readl(gbddata->gatttable + msg->offset + i - 1); /* PCI posting */

    /* Flush CPU cache - make sure data in GATT is up to date */
    flushcpucache();

    /* Flush GATT table at card */
    struct pHidd_AGPBridgeDevice_FlushGattTable fgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_FlushGattTable)
    };

    OOP_DoMethod(o, (OOP_Msg)&fgtmsg);

    ReleaseSemaphore(&gbddata->lock);
}

VOID METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, UnBindMemory)
{
    ULONG i;
    struct HIDDGenericBridgeDeviceData * gbddata = OOP_INST_DATA(cl, o);

    D(bug("Unbind offset %d, size %d\n", msg->offset, msg->size));

    if (gbddata->state != STATE_ENABLED)
        return;

    if (msg->size == 0)
        return;

    ObtainSemaphore(&gbddata->lock);

    /* TODO: get mask type */

    /* Remove entries from GATT table */
    for(i = 0; i < msg->size / 4096; i++)
    {
        writel((ULONG)(IPTR)gbddata->scratchmem, gbddata->gatttable + msg->offset + i);
    }
    
    readl(gbddata->gatttable + msg->offset + i - 1); /* PCI posting */
    
    /* Flush CPU cache - make sure data in GATT is up to date */
    flushcpucache();

    /* Flush GATT table */
    struct pHidd_AGPBridgeDevice_FlushGattTable fgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_FlushGattTable)
    };

    OOP_DoMethod(o, (OOP_Msg)&fgtmsg);
    
    ReleaseSemaphore(&gbddata->lock);
}

VOID METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, FlushChipset)
{
    /* This function is a NOOP */
}

BOOL METHOD(GenericBridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    bug("[GenericBridgeDevice] abstract Initialize method called\n");
    return FALSE;
}

