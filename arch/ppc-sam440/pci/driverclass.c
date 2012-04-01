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
    ( 0x80000000 | ((bus)<<16) |    \
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

ULONG ReadConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    ULONG temp;
    
    Disable();
    outl_le(CFGADD(bus, dev, sub, reg),PCIC0_CFGADDR);
    temp=inl_le(PCIC0_CFGDATA);
    //D(bug("[PCI440] -> %08x = %08x\n", CFGADD(bus, dev, sub, reg), temp));
    Enable();

    return temp;
}

ULONG PCI440__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

UWORD ReadConfigWord(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(psd, bus, dev, sub, reg);
    return temp.uw[1 - ((reg&2)>>1)];
}
    

UWORD PCI440__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    return ReadConfigWord(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

UBYTE PCI440__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o, 
                                            struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    pcicfg temp;

    temp.ul = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg); 
    return temp.ub[3 - (msg->reg & 3)];
}

void WriteConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UBYTE reg, ULONG val)
{
    Disable();
    outl_le(CFGADD(bus, dev, sub, reg),PCIC0_CFGADDR);
    outl_le(val,PCIC0_CFGDATA);
    Enable();
}

void PCI440__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

void PCI440__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    pcicfg temp;
    
    temp.ul = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
    temp.uw[1 - ((msg->reg&2)>>1)] = msg->val;
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, temp.ul);
}

void PCI440__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o,
                                            struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    pcicfg temp;
    
    temp.ul = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
    temp.ub[3 - (msg->reg & 3)] = msg->val;
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, temp.ul);
}

/* Class initialization and destruction */

static int PCI440_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci;
    
    D(bug("PCI440: Driver initialization\n"));

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
