/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI direct driver for i386 native.
    Lang: English
*/

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

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
static OOP_Object *pcidriver_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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

static ULONG pcidriver_RL(OOP_Class *cl, OOP_Object *o, 
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

static UWORD pcidriver_RW(OOP_Class *cl, OOP_Object *o, 
    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    pcicfg temp;

    temp.ul = pcidriver_RL(cl, o, (struct pHiddPCIDriver_ReadConfigLong *)msg);
    return temp.uw[(msg->reg&2)>>1];
}

static UBYTE pcidriver_RB(OOP_Class *cl, OOP_Object *o, 
    struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    pcicfg temp;

    temp.ul = pcidriver_RL(cl, o, (struct pHiddPCIDriver_ReadConfigLong *)msg); 
    return temp.ub[msg->reg & 3];
}

static void pcidriver_WL(OOP_Class *cl, OOP_Object *o,
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

#undef OOPBase
#undef SysBase
#undef UtilityBase

#define SysBase	    (psd->sysbase)
#define OOPBase	    (psd->oopbase)
#define UtilityBase (psd->utilitybase)

void free_pcidriverclass(struct pci_staticdata *psd, OOP_Class *cl)
{
    D(bug("PCI: Dummy Driver Class destruction\n"));
    
    if (psd)
    {
	OOP_RemoveClass(cl);

	if (cl)
	    OOP_DisposeObject((OOP_Object *)cl);
	
	OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
	OOP_ReleaseAttrBase(IID_Hidd);
    }
}
	
#define _NUM_ROOT_METHODS	1
#define _NUM_PCIDRIVER_METHODS	4

OOP_Class *init_pcidriverclass(struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;
    OOP_Object *pci = NULL;

    struct OOP_MethodDescr root_descr[_NUM_ROOT_METHODS + 1] = 
    {
	{ OOP_METHODDEF(pcidriver_new), moRoot_New },
	{ NULL, 0UL }
    };

    struct OOP_MethodDescr pcidriver_descr[_NUM_PCIDRIVER_METHODS + 1] =
    {
	{ OOP_METHODDEF(pcidriver_RB),  moHidd_PCIDriver_ReadConfigByte },
	{ OOP_METHODDEF(pcidriver_RW),  moHidd_PCIDriver_ReadConfigWord },
	{ OOP_METHODDEF(pcidriver_RL),  moHidd_PCIDriver_ReadConfigLong },
	{ OOP_METHODDEF(pcidriver_WL),  moHidd_PCIDriver_WriteConfigLong },
	{ NULL, 0UL }
    };

    struct OOP_InterfaceDescr ifdescr[] =
    {
	{ root_descr,	    IID_Root,		_NUM_ROOT_METHODS },
	{ pcidriver_descr,  IID_Hidd_PCIDriver,	_NUM_PCIDRIVER_METHODS },
	{ NULL, NULL, 0UL }
    };

    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
	{ aMeta_SuperID,	(IPTR)CLID_Hidd_PCIDriver },
	{ aMeta_InterfaceDescr,	(IPTR)ifdescr },
	{ aMeta_InstSize,	(IPTR)0 },
	{ TAG_DONE, 0UL }
    };

    D(bug("PCPCI: Driver initialization\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if (cl)
	{
	    cl->UserData = (APTR)psd;
	    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
	    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
	    if (psd->hiddPCIDriverAB)
	    {
		struct pHidd_PCI_AddHardwareDriver msg;
		
		OOP_AddClass(cl);
		D(bug("PCPCI: Driver Class OK\n"));
		
		msg.driverClass = cl;
		msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
		D(bug("PCPCI: Adding Driver to main the class OK\n"));

		pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
		OOP_DoMethod(pci, (OOP_Msg)&msg);
		OOP_DisposeObject(pci);
		psd->driverClass = cl;
	    }
	    else
	    {
		free_pcidriverclass(psd, cl);
		cl = NULL;
	    }
	}
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("PCPCI: Driver ClassPtr = %x\n", cl));

    return cl;
}

