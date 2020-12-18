/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "ata_pci_intern.h"
#include "ata_pci_controller.h"

OOP_Object *PCIATACtrllr__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct atapciBase *base = cl->UserData;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct PCIATACtrllrData *data = OOP_INST_DATA(cl, o);
        D(bug("[ATA:PCI:Ctrllr] %s: instance @ 0x%p\n", __func__, o));
        if (msg->attrList)
            data->ctrllrDevice.ref_Device = (APTR)GetTagData(aHW_Device, 0, msg->attrList);
    }
    return o;
}

void PCIATACtrllr__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct atapciBase *base = cl->UserData;
    struct PCIATACtrllrData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    HW_Switch(msg->attrID, idx)
    {
        case aoHW_Device:
            {
                *msg->storage = (IPTR)data->ctrllrDevice.ref_Device;
            }
            return;
        default:
            break;
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}
