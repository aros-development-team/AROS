/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386 native.
    Lang: English
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <resources/processor.h>
#include <proto/processor.h>

#include <aros/symbolsets.h>
#include <asm/amcc440.h>
#include <asm/io.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase

#define HiddPCIDriverAttrBase   (PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase            (PSD(cl)->hiddAB)

/*
    We overload the New method in order to introduce the Hidd Name and
    HardwareName attributes.
*/
OOP_Object *PCI460__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;
    
    struct TagItem mytags[] = {
        { aHidd_Name, (IPTR)"PCIUSB" },
        { aHidd_HardwareName, (IPTR)"AMCC460 Pseudo-PCI driver for USB access" },
        { TAG_DONE, 0 }
    };

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags;

    if (msg->attrList)
    {
        mytags[2].ti_Tag = TAG_MORE;
        mytags[2].ti_Data = (IPTR)msg->attrList;
    }
 
    msg = &mymsg;
 
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

const ULONG ohci0[256/4] = {
/* 0x00 */    0x046010e8, /* Device / Vendor */
/* 0x04 */    0x00b00007, /* Enable */
/* 0x08 */    0x0c031000, /* OHCI CLASSC */
/* 0x0c */    0x00800000, /* Multifunction */
/* 0x10 */    OHCI0_HCREV,    /* AMCC460's OHCI0 base address */
/* 0x14 */    0x00000000,
/* 0x18 */    0x00000000,
/* 0x1c */    0x00000000,
/* 0x20 */    0x00000000,
/* 0x24 */    0x00000000,
/* 0x28 */    0x00000000,
/* 0x2c */    0x00000000,
/* 0x30 */    0x00000000,
/* 0x34 */    0x00000000,
/* 0x38 */    0x00000000,
/* 0x3c */    0x01030100 | INTR_UIC2_USB_OHCI,
/* 0x40 */    0x00000000,
/* 0x44 */    0x00000000,
/* 0x48 */    0x00000000,
/* 0x4c */    0x00000000,
/* 0x50 */    0x00000000,
/* 0x54 */    0x00000000,
/* 0x58 */    0x00000000,
/* 0x5c */    0x00000000,
/* 0x60 */    0x00002020,       /* PORTWAKECAP / FLADJ / SBRN */
};

const ULONG ehci0[256/4] = {
/* 0x00 */    0x046010e8, /* Device / Vendor */
/* 0x04 */    0x00b00006, /* Enable */
/* 0x08 */    0x0c032000, /* EHCI CLASSC */
/* 0x0c */    0x00800000, /* Multifunction */
/* 0x10 */    EHCI0_HCCAPBASE,    /* AMCC460's EHCI0 base address */
/* 0x14 */    0x00000000,
/* 0x18 */    0x00000000,
/* 0x1c */    0x00000000,
/* 0x20 */    0x00000000,
/* 0x24 */    0x00000000,
/* 0x28 */    0x00000000,
/* 0x2c */    0x00000000,
/* 0x30 */    0x00000000,
/* 0x34 */    0x00000000,
/* 0x38 */    0x00000000,
/* 0x3c */    0x01030200 | INTR_UIC2_USB_EHCI,
/* 0x40 */    0x00000000,
/* 0x44 */    0x00000000,
/* 0x48 */    0x00000000,
/* 0x4c */    0x00000000,
/* 0x50 */    0x00000000,
/* 0x54 */    0x00000000,
/* 0x58 */    0x00000000,
/* 0x5c */    0x00000000,
/* 0x60 */    0x00002020,       /* PORTWAKECAP / FLADJ / SBRN */
};

ULONG ReadConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG temp = ~0;

    if (bus == 0 && dev == 0 && sub == 0) {
        /* Simulated PCI OHCI device, mapped to OHCI0 */
        temp = ohci0[reg>>2];
    }
    if (bus == 0 && dev == 0 && sub == 1) {
        /* Simulated PCI EHCI device, mapped to EHCI0 */
        temp = ehci0[reg>>2];
    }

    return temp;
}

ULONG PCI460__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

void WriteConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    /* Nothing to do */
}

void PCI460__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

/* Class initialization and destruction */
static inline ULONG GetPVR(void)
{
    struct Library *ProcessorBase = OpenResource(PROCESSORNAME);
    ULONG pvr = 0;

    if (ProcessorBase) {
        struct TagItem tags[] = {
            { GCIT_Model, (IPTR)&pvr },
            { TAG_END }
        };
        GetCPUInfo(tags);
    }

    return pvr;
}

static int PCI460_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci;
    ULONG pvr;
    
    struct pHidd_PCI_AddHardwareDriver msg,*pmsg=&msg;

    /* Are we on the right machine? */
    pvr = GetPVR();
    if (pvr != PVR_PPC460EX_B)
        return FALSE;
    
    D(bug("PCI460: Driver initialization\n"));

    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (LIBBASE->psd.hiddPCIDriverAB == 0 || LIBBASE->psd.hiddAB == 0)
    {
        D(bug("PCI460: ObtainAttrBases failed\n"));
        return FALSE;
    }

    msg.driverClass = LIBBASE->psd.driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("PCI460: Adding Driver to main the class OK\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)pmsg);
    OOP_DisposeObject(pci);

    D(bug("PCI460: All OK\n"));

    return TRUE;
}

static int PCI460_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("PCI460: Class destruction\n"));
    
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);
    
    return TRUE;
}
        
ADD2INITLIB(PCI460_InitClass, 0)
ADD2EXPUNGELIB(PCI460_ExpungeClass, 0)
