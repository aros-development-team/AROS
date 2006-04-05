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

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#undef HiddPCIDriverAttrBase
#undef HiddAttrBase

#define	HiddPCIDriverAttrBase	(PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase		(PSD(cl)->hiddAB)

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
OOP_Object *PCPCI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;
    
    struct TagItem mytags[] = {
	{ aHidd_Name, (IPTR)"PCINative" },
	{ aHidd_HardwareName, (IPTR)"IA32 native direct access PCI driver" },
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

static inline ULONG inl(UWORD port)
{
    ULONG val;

    asm volatile ("inl %w1,%0":"=a"(val):"Nd"(port));

    return val;
}

static inline void outl(ULONG val, UWORD port)
{
    asm volatile ("outl %0,%w1"::"a"(val),"Nd"(port));
}

ULONG PCPCI__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    ULONG orig,temp;
    
    Disable();
    orig=inl(PCI_AddressPort);
    outl(CFGADD(msg->bus, msg->dev, msg->sub, msg->reg),PCI_AddressPort);
    temp=inl(PCI_DataPort);
    outl(orig, PCI_AddressPort);
    Enable();

    return temp;
}

UWORD PCPCI__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    pcicfg temp;

    temp.ul = PCPCI__Hidd_PCIDriver__ReadConfigLong(cl, o, (struct pHidd_PCIDriver_ReadConfigLong *)msg);
    return temp.uw[(msg->reg&2)>>1];
}

UBYTE PCPCI__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o, 
					    struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    pcicfg temp;

    temp.ul = PCPCI__Hidd_PCIDriver__ReadConfigLong(cl, o, (struct pHidd_PCIDriver_ReadConfigLong *)msg); 
    return temp.ub[msg->reg & 3];
}

void PCPCI__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
					    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    ULONG orig;
    
    Disable();
    orig=inl(PCI_AddressPort);
    outl(CFGADD(msg->bus, msg->dev, msg->sub, msg->reg),PCI_AddressPort);
    outl(msg->val,PCI_DataPort);
    outl(orig, PCI_AddressPort);
    Enable();
}

/* Class initialization and destruction */

AROS_SET_LIBFUNC(PCPCI_InitClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT
    
    OOP_Object *pci;
    
    D(bug("PCPCI: Driver initialization\n"));

    struct pHidd_PCI_AddHardwareDriver msg;
    
    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (LIBBASE->psd.hiddPCIDriverAB == 0 || LIBBASE->psd.hiddAB == 0)
    {
	D(bug("PCPCI: ObtainAttrBases failed\n"));
	return FALSE;
    }
    
    msg.driverClass = LIBBASE->psd.driverClass;
    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    D(bug("PCPCI: Adding Driver to main the class OK\n"));

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    OOP_DoMethod(pci, (OOP_Msg)&msg);
    OOP_DisposeObject(pci);

    D(bug("PCPCI: All OK\n"));

    return TRUE;
    
    AROS_SET_LIBFUNC_EXIT
}

AROS_SET_LIBFUNC(PCPCI_ExpungeClass, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    D(bug("PCPCI: Class destruction\n"));
    
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);
    
    AROS_SET_LIBFUNC_EXIT
}
	
ADD2INITLIB(PCPCI_InitClass, 0)
ADD2EXPUNGELIB(PCPCI_ExpungeClass, 0)
