/*
    Copyright © 2017-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Elbox FastATA HIDD
    Lang: English
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <hardware/ata.h>
#include <hidd/ata.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <hardware/intbits.h>

#include "bus_class.h"
#include "interface_pio.h"

AROS_INTH1(IDE_Handler_FASTATA, struct ATA_BusData *, bus)
{
    AROS_INTFUNC_INIT

    D(bug("[ATA:FastATA] %s()\n", __func__);)

    return FALSE;

    AROS_INTFUNC_EXIT
}

static BOOL ata_CreateFastATAInterrupt(struct ATA_BusData *bus, UBYTE num)
{
    D(bug("[ATA:FastATA] %s()\n", __func__);)
    return FALSE;
}
static void ata_RemoveFastATAInterrupt(struct ATA_BusData *bus)
{
    D(bug("[ATA:FastATA] %s()\n", __func__);)
}

OOP_Object *FASTATA__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct ataBase *base = cl->UserData;
    struct ata_ProbedBus *bus = (struct ata_ProbedBus *)GetTagData(aHidd_DriverData, 0, msg->attrList);

    D(bug("[ATA:FastATA] %s()\n", __func__);)

    if (!bus)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct ATA_BusData *data = OOP_INST_DATA(cl, o);

        data->bus = bus;
        data->gaylebase = data->bus->port;
        data->gayleirqbase = data->bus->gayleirqbase;
        ata_CreateFastATAInterrupt(data, 0);
    }

    D(bug("[ATA:FastATA] %s: Instance @ %p\n", __func__, o);)
    return o;
}

void FASTATA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);

    D(bug("[ATA:FastATA] %s()\n", __func__);)

    ata_RemoveFastATAInterrupt(data);
    FreeVec(data->bus);

    OOP_DoSuperMethod(cl, o, msg);
}

void FASTATA__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct ataBase *base = cl->UserData;
    ULONG idx;

    D(bug("[ATA:FastATA] %s()\n", __func__);)

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

void FASTATA__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct ataBase *base = cl->UserData;
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tstate = msg->attrList;
    struct TagItem *tag;

    D(bug("[ATA:FastATA] %s()\n", __func__);)

    while ((tag = NextTagItem(&tstate)))
    {
        ULONG idx;

        Hidd_ATABus_Switch(tag->ti_Tag, idx)
        {
        case aoHidd_ATABus_IRQHandler:
            data->ata_HandleIRQ = (APTR)tag->ti_Data;
            break;

        case aoHidd_ATABus_IRQData:
            data->irqData = (APTR)tag->ti_Data;
            break;
        }
    }
}

APTR FASTATA__Hidd_ATABus__GetPIOInterface(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ATA_BusData *data = OOP_INST_DATA(cl, o);
    struct pio_data *pio = (struct pio_data *)OOP_DoSuperMethod(cl, o, msg);

    D(bug("[ATA:FastATA] %s()\n", __func__);)

    if (pio)
    {
        pio->port = data->bus->port;
        pio->altport  = data->bus->altport;
        pio->dataport = (UBYTE*)(((ULONG)pio->port) & ~3);
     }

    return pio;
}

BOOL FASTATA__Hidd_ATABus__SetXferMode(OOP_Class *cl, OOP_Object *obj, OOP_Msg msg)
{
    D(bug("[ATA:FastATA] %s()\n", __func__);)

    return TRUE;
}

void FASTATA__Hidd_ATABus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[ATA:FastATA] %s()\n", __func__);)

    OOP_DoSuperMethod(cl, o, msg);
}
