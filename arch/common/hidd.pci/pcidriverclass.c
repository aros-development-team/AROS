/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved.
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

#include <aros/symbolsets.h>

#include "pci.h"

#define DEBUG 1
#include <aros/debug.h>

#ifdef HiddPCIDriverAttrBase
#undef HiddPCIDriverAttrBase
#endif // HiddPCIDriverAttrBase

#define	HiddPCIDriverAttrBase	(PSD(cl)->hiddPCIDriverAB)
#define HiddAttrBase (PSD(cl)->hiddAB)

typedef union _pcicfg
{
    ULONG   ul;
    UWORD   uw[2];
    UBYTE   ub[4];
} pcicfg;

OOP_Object *PCIDrv__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
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
ULONG PCIDrv__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, 
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
void PCIDrv__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
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

UBYTE PCIDrv__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o, 
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

UWORD PCIDrv__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, 
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

void PCIDrv__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o,
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

void PCIDrv__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o,
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
APTR PCIDrv__Hidd_PCIDriver__CPUtoPCI(OOP_Class *cl, OOP_Object *o,
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
APTR PCIDrv__Hidd_PCIDriver__PCItoCPU(OOP_Class *cl, OOP_Object *o,
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
APTR PCIDrv__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_MapPCI *msg)
{
    /* Generic driver in case of DirecAccess PCI bus */
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    if (instance->DirectBus)
    {
	struct pHidd_PCIDriver_PCItoCPU mmsg, *pmmsg=&mmsg;
	mmsg.mID = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_PCItoCPU);
	mmsg.address = msg->PCIAddress;
	
	return ((APTR)OOP_DoMethod(o, (OOP_Msg)pmmsg));
    } else return (APTR)0xffffffff;
}

/*
    PCIDriver::UnmapPCI(Address, Length) unmaps the mapped PCI area previously
    allocated with MapPCI method.
*/
VOID PCIDrv__Hidd_PCIDriver__UnmapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_UnmapPCI *msg)
{
    /* Generic driver has nothing to do here */
}

/*
    PCIDriver::AllocPCIMemory(Size) allocates memory region available for
    PCI BusMaster devices.
*/
APTR PCIDrv__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o,
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
VOID PCIDrv__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_FreePCIMem *msg)
{
    APTR memory = *(APTR*)((IPTR)msg->Address - 4);
    FreeVec(memory);
}

VOID PCIDrv__Root__Get(OOP_Class *cl, OOP_Object *o,
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

static int PCIDrv_InitMIDs(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[PCIDriver] Dummy Driver initialization\n"));
    /*
     * We do have driver class. Now we can get some MethodID's,
     * so that whole PCI subsystem works slightly faster ;)
     */
		
    LIBBASE->psd.mid_RB = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigByte);
    LIBBASE->psd.mid_RW = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigWord);
    LIBBASE->psd.mid_RL = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_ReadConfigLong);
    
    LIBBASE->psd.mid_WB = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_WriteConfigByte);
    LIBBASE->psd.mid_WW = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_WriteConfigWord);
    LIBBASE->psd.mid_WL = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_WriteConfigLong);

    return TRUE;
}

ADD2INITLIB(PCIDrv_InitMIDs, 0)
