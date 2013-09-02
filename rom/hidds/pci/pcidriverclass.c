/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Base PCI driver class
    Lang: English

    I am not sure, whether this piece of code is already aware of endianess.
    Has to be checked soon ;)
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <aros/symbolsets.h>

#include "pci.h"

OOP_Object *PCIDrv__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    instance->DirectBus = GetTagData(aHidd_PCIDriver_DirectBus, TRUE, msg->attrList);
    instance->IOBase = GetTagData(aHidd_PCIDriver_IOBase, 0, msg->attrList);

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
    /*
     * First, read whole ConfigWord from PCI config space, using defined 
     * method
     */
    ULONG temp = HIDD_PCIDriver_ReadConfigLong(o, msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    // Then, return only this part of the Long which is requested
    return (temp >> ((msg->reg & 3) * 8)) & 0xff;
}

UWORD PCIDrv__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, 
    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    ULONG temp = HIDD_PCIDriver_ReadConfigLong(o, msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    return (temp >> ((msg->reg & 2) * 8)) & 0xffff;
}

void PCIDrv__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    ULONG temp;
    const int shift = (msg->reg & 3) * 8;

    // Read whole Long from PCI config space.
    temp = HIDD_PCIDriver_ReadConfigLong(o, msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    // Modify proper part of it according to request.
    temp = (temp & ~(0xff << shift)) | ((ULONG)msg->val << shift);

    // And put whole Long again into PCI config space.
    HIDD_PCIDriver_WriteConfigLong(o, msg->bus, msg->dev, msg->sub, msg->reg & ~3, temp);
}

void PCIDrv__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    ULONG temp;
    const int shift = (msg->reg & 2) * 8;

    // Read whole Long from PCI config space.
    temp = HIDD_PCIDriver_ReadConfigLong(o, msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    // Modify proper part of it according to request.
    temp = (temp & ~(0xffff << shift)) | ((ULONG)msg->val << shift);

    // And put whole Long again into PCI config space.
    HIDD_PCIDriver_WriteConfigLong(o, msg->bus, msg->dev, msg->sub, msg->reg & ~3, temp);
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
        return (APTR)msg->address;
    else
        return (APTR)-1;
}

/*
    PCIDriver::PCItoCPU() is opposite to above.
*/
APTR PCIDrv__Hidd_PCIDriver__PCItoCPU(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_PCItoCPU *msg)
{
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    if (instance->DirectBus)
        return (APTR)msg->address;
    else
        return (APTR)-1;
}

/*
    PCIDriver::MapPCI(Address, Length) maps the Length bytes of PCI address 
    space at Address to the CPU address space.
*/
APTR PCIDrv__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_MapPCI *msg)
{
    /*
     * Generic driver in case of DirecAccess PCI bus.
     * Our memory space is already mapped, but we may still need
     * to perform physical to virtual translation (in case if our
     * system uses virtual addressing).
     */
    struct DrvInstData *instance = (struct DrvInstData *)OOP_INST_DATA(cl, o);

    if (instance->DirectBus)
        return HIDD_PCIDriver_PCItoCPU(o, msg->PCIAddress);
    else
        return (APTR)-1;
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
    PCIDriver::AllocPCIMem(Size) allocates memory region available for
    PCI BusMaster devices.
*/
APTR PCIDrv__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_AllocPCIMem *msg)
{
    APTR memory = AllocVec(msg->Size + 4096 + AROS_ALIGN(sizeof(APTR)), MEMF_31BIT|MEMF_CLEAR);
    IPTR diff;
    
    diff = (IPTR)memory - (AROS_ROUNDUP2((IPTR)memory + sizeof(APTR), 4096));
    *((APTR*)((IPTR)memory - diff - sizeof(APTR))) = memory;

    return (APTR)((IPTR)memory - diff);
}

/*
    PCIDriver::FreePCIMemory(Address) frees previously allocated memory for PCI
    devices
*/
VOID PCIDrv__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_FreePCIMem *msg)
{
    APTR memory = *(APTR*)((IPTR)msg->Address - sizeof(APTR));
    FreeVec(memory);
}

static void krnIRQwrapper(void *data1, void *data2)
{
    struct Interrupt *irq = (struct Interrupt *)data1;

    AROS_INTC1(irq->is_Code, irq->is_Data);
}

/*****************************************************************************************

    NAME
        moHidd_PCIDriver_AddInterrupt

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_PCIDriver_AddInterrupt *Msg);

        OOP_Object *HIDD_PCIDriver_AddInterrupt(OOP_Object *obj, OOP_Object *device,
                                                struct Interrupt *interrupt);

    LOCATION
        CLID_Hidd_PCIDriver

    FUNCTION
        Add interrupt handler for the specified device.

        This method is present in order to provide abstraction for
        different PCI implementations. Default implementation of
        this method assumes 1:1 mapping between system interrupts
        and PCI interrupts. However, on some machines this is not
        true (an example is Amiga(tm) bridgeboards). In this case
        you will have to provide alternate implementation of this
        method.

    INPUTS
        obj       - Pointer to your driver object.
        device    - A pointer to the device object.
        interrupt - Interrupt structure to add.

    RESULT
        TRUE it succesful or FALSE on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_PCIDriver_RemoveInterrupt

    INTERNALS

*****************************************************************************************/

BOOL PCIDrv__Hidd_PCIDriver__AddInterrupt(OOP_Class *cl, OOP_Object *o,
     struct pHidd_PCIDriver_AddInterrupt *msg)
{
    IPTR irq;

    OOP_GetAttr(msg->device, aHidd_PCIDevice_INTLine, &irq);
    msg->interrupt->is_Node.ln_Succ =
        KrnAddIRQHandler(irq, krnIRQwrapper, msg->interrupt, NULL);
    return msg->interrupt->is_Node.ln_Succ ? TRUE : FALSE;
}

/*****************************************************************************************

    NAME
        moHidd_PCIDriver_RemoveInterrupt

    SYNOPSIS
        OOP_Object *OOP_DoMethod(OOP_Object *obj, struct pHidd_PCIDriver_RemoveInterrupt *Msg);

        OOP_Object *HIDD_PCIDriver_RemoveInterrupt(OOP_Object *obj, OOP_Object *device,
                                                   struct Interrupt *interrupt);

    LOCATION
        CLID_Hidd_PCIDriver

    FUNCTION
        Remove interrupt handler from the specified device.

        This method is present in order to provide abstraction for
        different PCI implementations. Default implementation of
        this method assumes 1:1 mapping between system interrupts
        and PCI interrupts. However, on some machines this is not
        true (an example is Amiga(tm) bridgeboards). In this case
        you will have to provide alternate implementation of this
        method.

    INPUTS
        obj       - Pointer to your driver object.
        device    - A pointer to the device object.
        interrupt - Interrupt structure to remove.

    RESULT
        None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        moHidd_PCIDriver_AddInterrupt

    INTERNALS

*****************************************************************************************/

VOID PCIDrv__Hidd_PCIDriver__RemoveInterrupt(OOP_Class *cl, OOP_Object *o,
     struct pHidd_PCIDriver_RemoveInterrupt *msg)
{
    KrnRemIRQHandler(msg->interrupt->is_Node.ln_Succ);
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

            case aoHidd_PCIDriver_IOBase:
                *msg->storage = (IPTR)instance->IOBase;
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
