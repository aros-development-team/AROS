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

#define CFGADD(bus,dev,func,reg)    \
    ( psd->CfgBase | ((bus)<<16) |    \
    ((dev)<<11) | ((func)<<8) | ((reg)&~3))

typedef union _pcicfg
{
    ULONG   ul;
    UWORD   uw[2];
    UBYTE   ub[4];
} pcicfg;

/*
    We overload the New method in order to introduce the Hidd Name and
    HardwareName attributes.
*/
OOP_Object *PCI440__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;
    
    struct TagItem mytags[] = {
        { aHidd_Name, (IPTR)"PCINative" },
        { aHidd_HardwareName, (IPTR)"AMCC440 native direct access PCI driver" },
        { aHidd_PCIDriver_IOBase, PCI0_IO },
        { TAG_DONE, 0 }
    };

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags[0];

    if (msg->attrList)
    {
        mytags[3].ti_Tag = TAG_MORE;
        mytags[3].ti_Data = (IPTR)msg->attrList;
    }
 
    msg = &mymsg;
 
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

static ULONG ReadConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    ULONG temp;
    
    Disable();
    outl_le(CFGADD(bus, dev, sub, reg),PCI0_CFGADDR);
    temp=inl_le(PCI0_CFGDATA);
    Enable();
#if 0
    /* FIXME: Is this really needed on SAM460? Other OSes do not seem to confirm this. */
    if (reg == 0x3c && psd->IntLine != 0xff) { /* PCICS_INT_LINE */
        temp &= ~0xff;
        temp |= psd->IntLine;
    }
#endif
    DB2(bug("[PCI440] -> %08x = %08x\n", CFGADD(bus, dev, sub, reg), temp));

    return temp;
}

ULONG PCI440__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

static void WriteConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    DB2(bug("[PCI440] <- %08x = %08x\n", CFGADD(bus, dev, sub, reg), val));
    Disable();
    outl_le(CFGADD(bus, dev, sub, reg),PCI0_CFGADDR);
    outl_le(val,PCI0_CFGDATA);
    Enable();
}

void PCI440__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
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

static int PCI440_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci;
    ULONG pvr;
    
    D(bug("PCI440: Driver initialization\n"));

    pvr = GetPVR();
    if (pvr == PVR_PPC460EX_B) {
        LIBBASE->psd.IntLine = INTR_UIC0_PCI0_IN;
        LIBBASE->psd.CfgBase = 0x00000000;
    } else {
        LIBBASE->psd.IntLine = 0xff;
        LIBBASE->psd.CfgBase = 0x80000000;
    }

    struct pHidd_PCI_AddHardwareDriver msg,*pmsg=&msg;
    
    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (LIBBASE->psd.hiddPCIDriverAB == 0 || LIBBASE->psd.hiddAB == 0)
    {
        D(bug("PCI440: ObtainAttrBases failed\n"));
        return FALSE;
    }

    msg.driverClass = LIBBASE->psd.driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("PCI440: Adding Driver to main the class OK\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)pmsg);
    OOP_DisposeObject(pci);

    D(bug("PCI440: CPU %p%p:%p%p PCI (0x%08x)\n",
                inl_le(PCI0_POM0LAH), inl_le(PCI0_POM0LAL),
                inl_le(PCI0_POM0PCIAH), inl_le(PCI0_POM0PCIAL),
                ~(inl_le(PCI0_POM0SA) & ~0xf) + 1
                ));
    D(bug("PCI440: CPU %p%p:%p%p PCI (0x%08x)\n",
                inl_le(PCI0_POM1LAH), inl_le(PCI0_POM1LAL),
                inl_le(PCI0_POM1PCIAH), inl_le(PCI0_POM1PCIAL),
                ~(inl_le(PCI0_POM1SA) & ~0xf) + 1
                ));
    uint64_t sa = ((uint64_t)inl_le(PCI0_PIM0SAH) << 32) | inl_le(PCI0_PIM0SAL);
    sa = ~(sa & ~0xfULL) + 1;
    D(bug("PCI440: PCI %p%p:%p%p CPU (0x%08x%08x)\n",
                inl_le(PCI0_BAR0H), inl_le(PCI0_BAR0L) & ~0xf,
                inl_le(PCI0_PIM0LAH), inl_le(PCI0_PIM0LAL),
                (uint32_t)(sa >>32), (uint32_t)sa
                ));

    D(bug("PCI440: All OK\n"));

    return TRUE;
}

static int PCI440_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    D(bug("PCI440: Class destruction\n"));
    
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);
    
    return TRUE;
}
        
ADD2INITLIB(PCI440_InitClass, 0)
ADD2EXPUNGELIB(PCI440_ExpungeClass, 0)
