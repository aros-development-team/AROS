/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Base PCI driver class
    Lang: English

    I am not sure, whether this piece of code is already aware of endianess.
    Has to be checked soon ;)
*/

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#ifdef HiddPCIDriverAttrBase
#undef HiddPCIDriverAttrBase
#endif // HiddPCIDriverAttrBase

#define	HiddPCIDriverAttrBase	(PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase (PSD(cl)->hiddAB)

#define SysBase (PSD(cl)->sysbase)
#define UtilityBase (PSD(cl)->utilitybase)

typedef union _pcicfg
{
    ULONG   ul;
    UWORD   uw[2];
    UBYTE   ub[4];
} pcicfg;

struct DrvInstData {
    BOOL DirectBus;
};

static OOP_Object *pcidriver_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    instance->DirectBus = GetTagData(aHidd_PCIDriver_DirectBus, TRUE, msg->attrList);

    return o;
}

/*
    ULONG PCIDriver::ReadConfigLong(bus, dev, sub, reg)

    This method is not implemented here (aka it should be the abstract class),
    and should be well defined in all PCI drivers.
*/
static ULONG pcidriver_RL(OOP_Class *cl, OOP_Object *o, 
    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    /* Wheeeee! Someone has forgotten to reimplement the ReadConfigLong!! */
    bug("[PCIDriver] Alert! PCIDriver::ReadConfigLong() unimplemented!!!\n");
    return 0xffffffff;
}

/*
    PCIDriver::WriteConfigLong(bus, dev, sub, reg, val)

    This method is not implemented here (aka it should be the abstract class),
    and should be well defined in all PCI drivers.
*/
static void pcidriver_WL(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    /* Wheeeee! Someone has forgotten to reimplement the WriteConfigLong!! */
    bug("[PCIDriver] Alert! PCIDriver::WriteConfigLong() unimplemented!!!\n");
}


/*
    Please note, that the following methods

    UWORD PCIDriver::ReadConfigWord()
    UBYTE PCIDriver::ReadConfigByte()
    VOID  PCIDriver::WriteConfigWord()
    VOID  PCIDriver::WriteConfigByte()

    *MAY* be implemented in driver class, but *DOESN'T HAVE TO*. I wrote small
    wrappers here using ReadConfigLong and WriteConfigLong in order to simplify
    developing of PCI drivers and reducing their size.
*/

static UBYTE pcidriver_RB(OOP_Class *cl, OOP_Object *o, 
    struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    pcicfg temp;
    struct pHidd_PCIDriver_ReadConfigLong mymsg;

    /*
	First, read whole ConfigWord from PCI config space, using defined 
	method
    */
    mymsg.mID = PSD(cl)->mid_RL;
    mymsg.bus = msg->bus;
    mymsg.dev = msg->dev;
    mymsg.sub = msg->sub;
    mymsg.reg = msg->reg & ~3;

    temp.ul = OOP_DoMethod(o, (OOP_Msg)&mymsg); 

    // Then, return only this part of the Long which is requested
    return temp.ub[msg->reg & 3];
}

static UWORD pcidriver_RW(OOP_Class *cl, OOP_Object *o, 
    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    pcicfg temp;
    struct pHidd_PCIDriver_ReadConfigLong mymsg;

    mymsg.mID = PSD(cl)->mid_RL;
    mymsg.bus = msg->bus;
    mymsg.dev = msg->dev;
    mymsg.sub = msg->sub;
    mymsg.reg = msg->reg & ~3;

    temp.ul = OOP_DoMethod(o, (OOP_Msg)&mymsg); 

    return temp.uw[(msg->reg&2)>>1];
}

static void pcidriver_WB(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    pcicfg temp;
    struct pHidd_PCIDriver_ReadConfigLong mymsg;
    struct pHidd_PCIDriver_WriteConfigLong mymsg2;

    // Read whole Long from PCI config space.
    mymsg.mID = PSD(cl)->mid_RL;
    mymsg.bus = msg->bus;
    mymsg.dev = msg->dev;
    mymsg.sub = msg->sub;
    mymsg.reg = msg->reg & ~3;

    temp.ul = OOP_DoMethod(o, (OOP_Msg)&mymsg); 

    // Modify proper part of it according to request.
    temp.ub[msg->reg & 3] = (UBYTE)msg->val;

    // And put whole Long again into PCI config space.
    mymsg2.mID = PSD(cl)->mid_WL;
    mymsg2.bus = msg->bus;
    mymsg2.dev = msg->dev;
    mymsg2.sub = msg->sub;
    mymsg2.reg = msg->reg & ~3;
    mymsg2.val = temp.ul;

    OOP_DoMethod(o, (OOP_Msg)&mymsg2);
}

static void pcidriver_WW(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    pcicfg temp;
    struct pHidd_PCIDriver_ReadConfigLong mymsg;
    struct pHidd_PCIDriver_WriteConfigLong mymsg2;

    // Read whole Long from PCI config space.
    mymsg.mID = PSD(cl)->mid_RL;
    mymsg.bus = msg->bus;
    mymsg.dev = msg->dev;
    mymsg.sub = msg->sub;
    mymsg.reg = msg->reg & ~3;

    temp.ul = OOP_DoMethod(o, (OOP_Msg)&mymsg); 

    // Modify proper part of it according to request.
    temp.uw[(msg->reg&2)>>1] = (UWORD)msg->val;

    // And put whole Long again into PCI config space.
    mymsg2.mID = PSD(cl)->mid_WL;
    mymsg2.bus = msg->bus;
    mymsg2.dev = msg->dev;
    mymsg2.sub = msg->sub;
    mymsg2.reg = msg->reg & ~3;
    mymsg2.val = temp.ul;

    OOP_DoMethod(o, (OOP_Msg)&mymsg2);

}

/*
    PCIDriver::CPUtoPCI() converts the address as seen by CPU into the
    address seen by the PCI bus. Do not reimplement this function, if
    CPU and PCI address spaces are equal on the machine you're writting
    driver for
*/
static APTR pcidriver_c2p(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_CPUtoPCI *msg)
{
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    if (instance->DirectBus)
    {
	return (APTR)msg->address;
    } else return (APTR)0xffffffff;
}

/*
    PCIDriver::PCItoCPU() is opposite to above.
*/
static APTR pcidriver_p2c(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_PCItoCPU *msg)
{
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    if (instance->DirectBus)
    {
	return (APTR)msg->address;
    } else return (APTR)0xffffffff;
}

/*
    PCIDriver::MapPCI(Address, Length) maps the Length bytes of PCI address 
    space at Address to the CPU address space.
*/
static APTR pcidriver_map(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_MapPCI *msg)
{
    /* Generic driver in case of DirecAccess PCI bus */
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    if (instance->DirectBus)
    {
	return msg->PCIAddress;
    } else return (APTR)0xffffffff;
}

/*
    PCIDriver::UnMapPCI(Address, Length) unmaps the mapped PCI area previously
    allocated with MapPCI method.
*/
static VOID pcidriver_umap(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_UnMapPCI *msg)
{
    /* Generic driver has nothing to do here */
}

/*
    PCIDriver::AllocPCIMemory(Size) allocates memory region available for
    PCI BusMaster devices.
*/
static APTR pcidriver_alloc(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_AllocPCIMem *msg)
{
    APTR memory = AllocVec(msg->Size + 4096 + AROS_ALIGN(sizeof(APTR)), MEMF_CLEAR);
    IPTR diff;
    
    diff = (IPTR)memory - (AROS_ROUNDUP2((IPTR)memory + 4, 4096));
    *((APTR*)((IPTR)memory - diff - 4)) = memory;

    return (APTR)((IPTR)memory - diff);
}

/*
    PCIDriver::FreePCIMemory(Address) frees previously allocated memory for PCI
    devices
*/
static VOID pcidriver_free(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_FreePCIMem *msg)
{
    APTR memory = *(APTR*)((IPTR)msg->Address - 4);
    FreeVec(memory);
}

static VOID pcidriver_get(OOP_Class *cl, OOP_Object *o,
    struct pRoot_Get *msg)
{
    ULONG idx;
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    if (IS_PCIDRV_ATTR(msg->attrID, idx))
    {
	switch(idx)
	{
	    case aoHidd_PCIDriver_DirectBus:
		*msg->storage = (IPTR)instance->DirectBus;
		break;
	    
	    default:
		OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
		break;
	}
    }
    else
    {
	OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
    }
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
    D(bug("[PCIDriver] Dummy Class destruction\n"));
    
    if (psd)
    {
	OOP_RemoveClass(cl);

	if (cl)
	    OOP_DisposeObject((OOP_Object *)cl);
    }
}
	
#define _NUM_ROOT_METHODS	2
#define _NUM_PCIDRIVER_METHODS	NUM_PCIDRIVER_METHODS

OOP_Class *init_pcidriverclass(struct pci_staticdata *psd)
{
    OOP_Class *cl = NULL;

    /* In case of regular driver, the New method should be written. Here not ;) */
    struct OOP_MethodDescr root_descr[_NUM_ROOT_METHODS + 1] = 
    {
	{ OOP_METHODDEF(pcidriver_new),	    moRoot_New },
	{ OOP_METHODDEF(pcidriver_get),	    moRoot_Get },
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
	{ OOP_METHODDEF(pcidriver_c2p), moHidd_PCIDriver_CPUtoPCI },
	{ OOP_METHODDEF(pcidriver_p2c), moHidd_PCIDriver_PCItoCPU },
	{ OOP_METHODDEF(pcidriver_map), moHidd_PCIDriver_MapPCI },
	{ OOP_METHODDEF(pcidriver_umap),moHidd_PCIDriver_UnMapPCI },
	{ OOP_METHODDEF(pcidriver_alloc),moHidd_PCIDriver_AllocPCIMem },
	{ OOP_METHODDEF(pcidriver_free),moHidd_PCIDriver_FreePCIMem },
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
	{ aMeta_SuperID,	(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,	(IPTR)ifdescr },
	{ aMeta_InstSize,	(IPTR)sizeof(struct DrvInstData) },
	{ aMeta_ID,		(IPTR)CLID_Hidd_PCIDriver },
	{ TAG_DONE, 0UL }
    };

    D(bug("[PCIDriver] Dummy Driver initialization\n"));

    if (MetaAttrBase)
    {
	cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
	if (cl)
	{
	    cl->UserData = (APTR)psd;

	    OOP_AddClass(cl);
	    D(bug("[PCIDriver] Dummy Driver Class OK\n"));
	    psd->pciDriverClass = cl;

	    /*
		We do have driver class. Now we can get some MethodID's,
		so that whole PCI subsystem works slightly faster ;)
	    */
		
	    psd->mid_RB = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigByte);
	    psd->mid_RW = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigWord);
	    psd->mid_RL = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigLong);
	
	    psd->mid_WB = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_WriteConfigByte);
	    psd->mid_WW = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_WriteConfigWord);
	    psd->mid_WL = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_WriteConfigLong);
	}
	OOP_ReleaseAttrBase(IID_Meta);
    }

    D(bug("[PCIDriver] ClassPtr = 0x%08x\n", cl));

    return cl;
}

