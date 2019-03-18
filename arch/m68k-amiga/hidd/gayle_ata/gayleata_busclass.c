/*
    Copyright © 2013-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: A600/A1200/A4000 ATA HIDD
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>

#include <hardware/ata.h>
#include <hidd/bus.h>
#include <hidd/ata.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <hardware/intbits.h>

#include "bus_class.h"
#include "interface_pio.h"

AROS_INTH1(IDE_Handler_A1200, struct ATA_BusData *, bus)
{
    AROS_INTFUNC_INIT

    volatile UBYTE *irqbase = bus->gayleirqbase;
    UBYTE irqmask = *irqbase;
    if (irqmask & GAYLE_IRQ_IDE) {
        volatile UBYTE *port;
        UBYTE status;

        port = bus->gaylebase;
        status = port[ata_Status * 4];
        /* Clear A600/A1200 IDE interrupt. (Stupid Gayle hardware)
         * Technically this should be done while interrupts are
         * disabled
         */
        *irqbase = 0x7c | (*irqbase & 3);
        if (status & ATAF_BUSY) {
            bug("[ATA:Gayle] ATA interrupt but BUSY flag set!?\n");
            return FALSE;
        }
        bus->ata_HandleIRQ(status, bus->irqData);
        return TRUE;
    }
    return FALSE;

    AROS_INTFUNC_EXIT
}

AROS_INTH1(IDE_Handler_A4000, struct ATA_BusData *, bus)
{
    AROS_INTFUNC_INIT

    /* A4000 interrupt clears when register is read */
    volatile UWORD *irqbase = (UWORD*)bus->gayleirqbase;
    UWORD irqmask = *irqbase;
    if (irqmask & (GAYLE_IRQ_IDE << 8)) {
        volatile UBYTE *port;
        UBYTE status;

        port = bus->gaylebase;
        status = port[ata_Status * 4];
        if (status & ATAF_BUSY) {
            bug("[ATA:Gayle] ATA interrupt but BUSY flag set!?\n");
            return FALSE;
        }
        bus->ata_HandleIRQ(status, bus->irqData);
        return TRUE;
    }
    return FALSE;

    AROS_INTFUNC_EXIT
}

static BOOL ata_CreateGayleInterrupt(struct ATA_BusData *bus, UBYTE num)
{
    struct Interrupt *irq = &bus->ideint;

    if (bus->bus->a4000) {
        irq->is_Code = (APTR)IDE_Handler_A4000;
    } else {
        bus->gayleintbase = (UBYTE*)GAYLE_INT_1200;
        irq->is_Code = (APTR)IDE_Handler_A1200;
    }

    irq->is_Node.ln_Pri = 20;
    irq->is_Node.ln_Type = NT_INTERRUPT;
    irq->is_Node.ln_Name = "AT-IDE";
    irq->is_Data = bus;
    AddIntServer(INTB_PORTS, irq);
    
    if (bus->gayleintbase) {
        volatile UBYTE *gayleintbase = bus->gayleintbase;
        *gayleintbase |= GAYLE_INT_IDE;
    }
    bus->ideintadded = TRUE;

    return TRUE;
}

static void ata_RemoveGayleInterrupt(struct ATA_BusData *bus)
{
    struct Interrupt *irq = &bus->ideint;

    if (!bus->ideintadded)
        return;
    bus->ideintadded = FALSE;
    if (bus->gayleintbase) {
        volatile UBYTE *gayleintbase = bus->gayleintbase;
        *gayleintbase &= ~GAYLE_INT_IDE;
    }
    RemIntServer(INTB_PORTS, irq);
}

/* Class Methods */

OOP_Object *GAYLEATA__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct ataBase *base = cl->UserData;
    struct ata_ProbedBus *bus = (struct ata_ProbedBus *)GetTagData(aHidd_DriverData, 0, msg->attrList);

    D(bug("[ATA:Gayle] %s()\n", __func__);)

    if (!bus)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &msg->mID);
    if (o)
    {
        struct ATA_BusData *data = OOP_INST_DATA(cl, o);
        //OOP_MethodID mDispose;

        data->bus = bus;
        data->gaylebase = data->bus->port;
        data->gayleirqbase = data->bus->gayleirqbase;
        ata_CreateGayleInterrupt(data, 0);

        //mDispose = msg->mID - moRoot_New + moRoot_Dispose;
        //OOP_DoSuperMethod(cl, o, &mDispose);
    }
    D(bug("[ATA:Gayle] %s: Instance @ %p\n", __func__, o);)
    return o;
}

void GAYLEATA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    //struct ataBase *base = cl->UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);

    D(bug("[ATA:Gayle] %s()\n", __func__);)

    ata_RemoveGayleInterrupt(data);
    FreeVec(data->bus);

    OOP_DoSuperMethod(cl, o, msg);
}

void GAYLEATA__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ataBase *base = cl->UserData;
    //struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    D(bug("[ATA:Gayle] %s()\n", __func__);)

    Hidd_ATABus_Switch(msg->attrID, idx)
    {
    case aoHidd_ATABus_Use80Wire:
        *msg->storage = FALSE;
        return;

    case aoHidd_ATABus_UseDMA:
        *msg->storage = FALSE;
        return;

    case aoHidd_ATABus_Use32Bit:
        *msg->storage = TRUE;
        return;
    }

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void GAYLEATA__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct ataBase *base = cl->UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tstate = msg->attrList;
    struct TagItem *tag;

    D(bug("[ATA:Gayle] %s()\n", __func__);)

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        Hidd_Bus_Switch(tag->ti_Tag, idx)
        {
        case aoHidd_Bus_IRQHandler:
            data->ata_HandleIRQ = (APTR)tag->ti_Data;
            break;

        case aoHidd_Bus_IRQData:
            data->irqData = (APTR)tag->ti_Data;
            break;
        }
    }
}

APTR GAYLEATA__Hidd_ATABus__GetPIOInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct pio_data *pio = (struct pio_data *)OOP_DoSuperMethod(cl, o, msg);
    
    D(bug("[ATA:Gayle] %s()\n", __func__);)

    if (pio)
    {
        pio->port = data->bus->port;
        pio->altport  = data->bus->altport;
        pio->dataport = (UBYTE*)(((ULONG)pio->port) & ~3);
     }

    return pio;
}

void GAYLEATA__Hidd_ATABus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    //struct ATA_BusData *data = OOP_INST_DATA(cl, o);

    OOP_DoSuperMethod(cl, o, msg);
}
