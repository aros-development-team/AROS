/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
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

#define GFX_INTEL_I915_REGSADDR         0x10
#define GFX_INTEL_I915_GATTADDR         0x1C
#define GFX_INTEL_I915_GMADDR           0x18
#define GFX_INTEL_I810_PGETBL_CTL       0x2020
#define GFX_INTEL_I810_PGETBL_ENABLED   0x00000001
#define AGP_INTEL_I830_GMCH_CTRL        0x52
#define AGP_INTEL_I830_GMCH_ENABLED     0x4

#define IS_915_BRIDGE(x)                \
(                                       \
    (x == 0x2588) || /* 915 */          \
    (x == 0x2580) || /* 82915G_HB */    \
    (x == 0x2590) || /* 82915GM_HB */   \
    (x == 0x2770) || /* 82945G_HB */    \
    (x == 0x27A0) || /* 82945GM_HB */   \
    (x == 0x27AC)    /* 82945GME_HB */  \
)                                       \

#define IS_915_GFX(x)                   \
(                                       \
    (x == 0x258a) || /* 915 */          \
    (x == 0x2582) || /* 82915G_IG */    \
    (x == 0x2592) || /* 82915GM_IG */   \
    (x == 0x2772) || /* 82945G_IG */    \
    (x == 0x27A2) || /* 82945GM_IG */   \
    (x == 0x27AE)    /* 82945GME_IG */  \
)  

/* NON-PUBLIC METHODS */
VOID METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, FlushGattTable)
{
    /* This function is a NOOP */
}

BOOL METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, CreateGattTable)
{
    struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);

    /* The GATT table is automatically created during POST - just use it */
    OOP_Object * igpdev = i915bddata->igp;
    /* ULONG gattsize = 256 * 1024; */ /* TODO: 1MB for G33 */
    ULONG gatttableaddr = 0; /* Address of GATT table */
    ULONG registersaddr = 0; /* Address of registers */
    
    gatttableaddr = readconfiglong(igpdev, GFX_INTEL_I915_GATTADDR);
    registersaddr = readconfiglong(igpdev, GFX_INTEL_I915_REGSADDR);
    registersaddr &= 0xfff80000;
    
    /* TODO: PCI MAP: agpsd->intelgatttable = (gatttableaddr, gatttablesize) */
    i915bddata->gatttable = (ULONG*)(IPTR)gatttableaddr;
    
    /* TODO: PCI MAP: agpsd->intelregs = (registersaddr, 128 * 4096) */
    i915bddata->regs = (UBYTE*)(IPTR)registersaddr;
    
//    ???? = (ULONG*)(readl(i915bddata->intelregs + GFX_INTEL_I810_PGETBL_CTL) & 0xfffff000);

    D(bug("[AGP] [Intel 915] Intel GATT table 0x%x, Regs 0x%x\n",
        (ULONG)(IPTR)i915bddata->gatttable, (ULONG)(IPTR)i915bddata->regs));

    flushcpucache();
    
    /* Create scratch page */
    i915bddata->scratchmembuffer = AllocVec(4096 * 2, MEMF_PUBLIC | MEMF_CLEAR);
    i915bddata->scratchmem = (ULONG*)(ALIGN((IPTR)i915bddata->scratchmembuffer, 4096));
    D(bug("[AGP] [Intel 915] Created scratch memory at 0x%x\n", (ULONG)(IPTR)i915bddata->scratchmem));
    
    /* TODO: Detect the amount of stolen memory */
    i915bddata->firstgattentry = 0;
    
    return TRUE;
}

BOOL METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, ScanAndDetectDevices)
{
    /* TODO: IMPLEMENT */
    /* TODO: Check if interrupt is set */
    return FALSE;
}

/* PUBLIC METHODS */
OOP_Object * METHOD(i915BridgeDevice, Root, New)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    
    if (o)
    {
        struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);
        i915bddata->flushpage = NULL;
        i915bddata->state = STATE_UNKNOWN;
        InitSemaphore(&i915bddata->lock);
        i915bddata->gatttable = NULL;
        i915bddata->scratchmem = NULL;
        i915bddata->bridgemode = 0;
        i915bddata->bridgeaperbase = 0;
        i915bddata->bridgeapersize = 0;
        i915bddata->regs = 0;
        i915bddata->igp = NULL;
        i915bddata->scratchmembuffer = NULL;
    }

    return o;
}

VOID i915BridgeDevice__Root__Dispose(OOP_Class * cl, OOP_Object * o, OOP_Msg msg)
{
    struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);

    /* Free GATT table -> NOOP */

    /* TODO: unmap flush page */
    /* TODO: release flush page resource */
    /* TODO: unmap agpsd->intelgatttable */
    /* TODO: unmap agpsd->intelregs */

    FreeVec(i915bddata->scratchmembuffer);
    i915bddata->scratchmembuffer = NULL;
    i915bddata->scratchmem = NULL;

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID METHOD(i915BridgeDevice, Root, Get)
{
    ULONG idx;
    struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);

    if (IS_AGPBRIDGEDEV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_AGPBridgeDevice_Mode:
                *msg->storage = i915bddata->bridgemode;
                return;

            case aoHidd_AGPBridgeDevice_ApertureBase:
                *msg->storage = i915bddata->bridgeaperbase;
                return;

            case aoHidd_AGPBridgeDevice_ApertureSize:
                *msg->storage = i915bddata->bridgeapersize;
                return;
        }
    }
    
    /* Use parent class for all other properties */
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, Enable)
{
    /* This function is a NOOP */
    return TRUE;
}

VOID METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, FlushChipset)
{
    struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);
    
    if (i915bddata->flushpage)
        writel(1, i915bddata->flushpage);
}

VOID METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, UnBindMemory)
{
    ULONG i;
    struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);

    D(bug("[AGP] [Intel 915] Unbind offset %d, size %d\n", msg->offset, msg->size));

    if (i915bddata->state != STATE_ENABLED)
        return;

    if (msg->size == 0)
        return;

    ObtainSemaphore(&i915bddata->lock);

    /* TODO: Check if offset is not before intelfirstgattentry */

    /* Remove entries from GATT table */
    for(i = 0; i < msg->size / 4096; i++)
    {
        writel((ULONG)(IPTR)i915bddata->scratchmem, i915bddata->gatttable + msg->offset + i);
    }
    
    readl(i915bddata->gatttable + msg->offset + i - 1); /* PCI posting */
    
    /* Flush CPU cache - make sure data in GATT is up to date */
    flushcpucache();

    /* Flush GATT table */
    struct pHidd_AGPBridgeDevice_FlushGattTable fgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_FlushGattTable)
    };

    OOP_DoMethod(o, (OOP_Msg)&fgtmsg);
    
    ReleaseSemaphore(&i915bddata->lock);
}

VOID METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, BindMemory)
{
    ULONG i;
    ULONG mask = 0x00000001;
    struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);

    D(bug("[AGP] [Intel 915] Bind address 0x%x into offset %d, size %d\n", (ULONG)msg->address, msg->offset, msg->size));

    if (i915bddata->state != STATE_ENABLED)
        return;

    ObtainSemaphore(&i915bddata->lock);

    /* TODO: Check if offset is not before intelfirstgattentry */

    /* TODO: check if offset + size / 4096 ends before gatt_table end */
    
    /* TODO: get mask type - check if mast type is of supported type */

    /* Flush incomming memory - will be done in flush_cpu_cache below */

    /* Additional mask for cached memory */    
    if (msg->type ==     vHidd_AGP_CachedMemory)
        mask |= 0x00000006;
    
    /* Insert entries into GATT table */
    for(i = 0; i < msg->size / 4096; i++)
    {
        /* Write masked memory address into GATT */
        writel((msg->address + (4096 * i)) | mask,
            i915bddata->gatttable + msg->offset + i);
    }
    
    readl(i915bddata->gatttable + msg->offset + i - 1); /* PCI posting */

    /* Flush CPU cache - make sure data in GATT is up to date */
    flushcpucache();

    /* Flush GATT table at card */
    struct pHidd_AGPBridgeDevice_FlushGattTable fgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_FlushGattTable)
    };

    OOP_DoMethod(o, (OOP_Msg)&fgtmsg);

    ReleaseSemaphore(&i915bddata->lock);
}

#undef HiddPCIDeviceAttrBase
#define HiddPCIDeviceAttrBase   (SD(cl)->hiddPCIDeviceAB)

BOOL METHOD(i915BridgeDevice, Hidd_AGPBridgeDevice, Initialize)
{
    struct HIDDi915BridgeDeviceData * i915bddata = OOP_INST_DATA(cl, o);
    OOP_Object * bridgedev = NULL;
    OOP_Object * igpdev = NULL;
    UWORD gmchctrl = 0;
    ULONG entries = 0, i = 0;

    struct pHidd_AGPBridgeDevice_ScanAndDetectDevices saddmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_ScanAndDetectDevices)
    };
    
    /* Scan for bridge and igp devices */
    if (!OOP_DoMethod(o, (OOP_Msg)&saddmsg))
        return FALSE;

    igpdev = i915bddata->igp;
//TODO:    bridgedev = ???


    /* Initialize */


    /* Getting GART size - from video card */
    OOP_GetAttr(igpdev, aHidd_PCIDevice_Size2, (APTR)&i915bddata->bridgeapersize);
    
    D(bug("[AGP] [Intel 915] Read aperture size: %d MB\n", (ULONG)i915bddata->bridgeapersize));

    /* Creation of GATT table */
    struct pHidd_AGPBridgeDevice_CreateGattTable cgtmsg = {
    mID: OOP_GetMethodID(IID_Hidd_AGPBridgeDevice, moHidd_AGPBridgeDevice_CreateGattTable)
    };
    if (!OOP_DoMethod(o, (OOP_Msg)&cgtmsg))
        return FALSE;

    /* Getting GART base */
    i915bddata->bridgeaperbase = (IPTR)readconfiglong(igpdev, GFX_INTEL_I915_GMADDR);
    i915bddata->bridgeaperbase &= (~0x0fUL) /* PCI_BASE_ADDRESS_MEM_MASK */;
    D(bug("[AGP] [Intel 915] Reading aperture base: 0x%x\n", (ULONG)i915bddata->bridgeaperbase));
    
    /* Enabled GATT */
    gmchctrl = readconfigword(bridgedev, AGP_INTEL_I830_GMCH_CTRL);
    gmchctrl |= AGP_INTEL_I830_GMCH_ENABLED;
    writeconfigword(bridgedev, AGP_INTEL_I830_GMCH_CTRL, gmchctrl);

    /* FIXME: gatttable means differnt thing now - this is old code */
/*    writel((ULONG)i915bddata->gatttable | GFX_INTEL_I810_PGETBL_ENABLED,
        i915bddata->regs + GFX_INTEL_I810_PGETBL_CTL);*/

    readl(i915bddata->regs + GFX_INTEL_I810_PGETBL_CTL); /* PCI Posting */
    
    /* Bind GATT to scratch page */
    entries = i915bddata->bridgeapersize * 1024 * 1024 / 4096;
    for (i = i915bddata->firstgattentry; i < entries; i++)
    {
        writel((ULONG)(IPTR)i915bddata->scratchmem, i915bddata->gatttable + i);
    }
    readl(i915bddata->gatttable + i - 1);   /* PCI Posting. */
    
    flushcpucache();

    /* TODO: Setup chipset flushing */
    i915bddata->flushpage = NULL;
    
    
    /* THIS CLASS IS NOT FULLY IMPLEMENT AND NOT TESTED AT ALL */
    return FALSE;
}
