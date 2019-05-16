/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/kernel.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include <hidd/bus.h>
#include <hidd/scsi.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <interface/Hidd.h>

#include "wd33c93_intern.h"

OOP_Object *SCSIWD33C93Bus__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct scsiwd33c93Base *base = cl->UserData;
    struct wd33c93ProbedBus *pBus = (struct wd33c93ProbedBus *)GetTagData(aHidd_DriverData, 0, msg->attrList);
    D(bug("[SCSI:WD33C93:Bus] %s()\n", __PRETTY_FUNCTION__));

    if (!pBus)
        return NULL;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct SCSIWD33C93BusData *data = OOP_INST_DATA(cl, o);
        OOP_MethodID mDispose;

        D(bug("[SCSI:WD33C93:Bus] %s: instance @ 0x%p\n", __PRETTY_FUNCTION__, o));

        return o;

        mDispose = msg->mID - moRoot_New + moRoot_Dispose;
        OOP_DoSuperMethod(cl, o, &mDispose);
    }
    return NULL;
}

void SCSIWD33C93Bus__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct scsiwd33c93Base *base = cl->UserData;
    struct SCSIWD33C93BusData *data = OOP_INST_DATA(cl, o);

    D(bug("[SCSI:WD33C93:Bus] %s()\n", __PRETTY_FUNCTION__));

    OOP_DoSuperMethod(cl, o, msg);
}

void SCSIWD33C93Bus__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct scsiwd33c93Base *base = cl->UserData;
    struct SCSIWD33C93BusData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

#if (0)
    Hidd_SCSIBus_Switch(msg->attrID, idx)
    {
    }
#endif

    OOP_DoSuperMethod(cl, o, &msg->mID);
}

void SCSIWD33C93Bus__Root__Set(OOP_Class *cl, OOP_Object *o, struct pRoot_Set *msg)
{
    struct scsiwd33c93Base *base = cl->UserData;
    struct SCSIWD33C93BusData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tstate = msg->attrList;
    struct TagItem *tag;

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

void SCSIWD33C93Bus__Hidd_SCSIBus__Shutdown(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct SCSIWD33C93BusData *data = OOP_INST_DATA(cl, o);

    OOP_DoSuperMethod(cl, o, msg);
}
