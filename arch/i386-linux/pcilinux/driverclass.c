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

#define	HiddPCIDriverAB	(PSD(cl)->hiddPCIDriverAB)

#define CFGADD(bus,dev,func,reg)    \
    ( 0x80000000 | ((bus)<<16) |    \
    ((dev)<<11) | ((func)<<8) | ((reg)&~3))

typedef union _pcicfg
{
    ULONG   ul;
    UWORD   uw[2];
    UBYTE   ub[4];
} pcicfg;

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
    
    orig=inl(PCI_AddressPort);
    outl(CFGADD(msg->bus, msg->dev, msg->sub, msg->reg),PCI_AddressPort);
    temp=inl(PCI_DataPort);
    outl(orig, PCI_AddressPort);

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
    
    orig=inl(PCI_AddressPort);
    outl(CFGADD(msg->bus, msg->dev, msg->sub, msg->reg),PCI_AddressPort);
    outl(msg->val,PCI_DataPort);
    outl(orig, PCI_AddressPort);
}

static void pcidriver_WW(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigWord *msg)
{
}

static void pcidriver_WB(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigByte *msg)
{
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
    }
}
	
#define _NUM_ROOT_METHODS	0
#define _NUM_PCIDRIVER_METHODS	NUM_PCIDRIVER_METHODS

OOP_Class *init_pcidriverclass(struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;
    OOP_Object *pci = NULL;

    struct OOP_MethodDescr root_descr[_NUM_ROOT_METHODS + 1] = 
    {
	{ NULL, 0UL }
    };

    struct OOP_MethodDescr pcidriver_descr[_NUM_PCIDRIVER_METHODS + 1] =
    {
	{ OOP_METHODDEF(pcidriver_RB),  moHidd_PCIDriver_ReadConfigByte },
	{ OOP_METHODDEF(pcidriver_RW),  moHidd_PCIDriver_ReadConfigWord },
	{ OOP_METHODDEF(pcidriver_RL),  moHidd_PCIDriver_ReadConfigLong },
	{ OOP_METHODDEF(pcidriver_WB),  moHidd_PCIDriver_WriteConfigByte },
	{ OOP_METHODDEF(pcidriver_WW),  moHidd_PCIDriver_WriteConfigWord },
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

    D(bug("LinuxPCI: Driver initialization\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if (cl)
	{
	    cl->UserData = (APTR)psd;
	    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
	    if (psd->hiddPCIDriverAB)
	    {
		struct pHidd_PCI_AddHardwareDriver msg;
		
		OOP_AddClass(cl);
		D(bug("LinuxPCI: Driver Class OK\n"));
		
		msg.managed = 1;
		msg.driverClass = cl;
		msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
		D(bug("LinuxPCI: Adding Driver to main the class OK\n"));

		pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
		OOP_DoMethod(pci, (OOP_Msg)&msg);
		OOP_DisposeObject(pci);
	    }
	    else
	    {
		free_pcidriverclass(psd, cl);
		cl = NULL;
	    }
	}
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("LinuxPCI: Driver ClassPtr = %x\n", cl));

    return cl;
}

