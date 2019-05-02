/*
    Copyright (C) 2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/utility.h>

#include <hidd/storage.h>
#include <hidd/scsi.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "scsi.h"

const char scsi_DevName[] = "SCSI Controller";

OOP_Object *SCSI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct scsiBase *SCSIBase = cl->UserData;

    OOP_Object *scsiController = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (scsiController)
    {
        struct scsi_Controller *data = OOP_INST_DATA(cl, scsiController);

        /*register the controller in ata.device */
        D(bug ("[SCSI:Controller] Root__New: Controller Entry @ 0x%p\n", data);)

        data->sc_Class = cl;
        data->sc_Object = scsiController;

        AddTail(&SCSIBase->scsi_Controllers, &data->sc_Node);
    }
    return scsiController;
}

VOID SCSI__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct scsiBase *SCSIBase = cl->UserData;
    struct scsi_Controller *scsiNode, *tmpNode;

    D(bug ("[SCSI:Controller] Root__Dispose(0x%p)\n", o);)

    ForeachNodeSafe (&SCSIBase->scsi_Controllers, scsiNode, tmpNode)
    {
        if (scsiNode->sc_Object == o)
        {
            D(bug ("[SCSI:Controller] Root__Dispose: Destroying Controller Entry @ 0x%p\n", scsiNode);)
            Remove(&scsiNode->sc_Node);
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            return;
        }
    }
}

void SCSI__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
#if (0)
    struct scsi_Controller *data = OOP_INST_DATA(cl, o);
    IPTR idx;

    if (IS_SCSI_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_SCSI_xxx:
            *msg->storage = (IPTR)data->yyyy;
            return;
        }
    }
#endif
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL SCSI__Hidd_StorageController__RemoveBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_RemoveBus *msg)
{
    D(bug ("[SCSI:Controller] Hidd_StorageController__RemoveBus(0x%p)\n", msg->busObject);)
   /*
     * Currently we don't support unloading SCSI bus drivers.
     * This is a very-very big TODO.
     */
    return FALSE;
}

BOOL SCSI__Hidd_StorageController__SetUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_SetUpBus *msg)
{
    struct scsiBase *SCSIBase = cl->UserData;

    D(bug ("[SCSI:Controller] Hidd_StorageController__SetUpBus(0x%p)\n", msg->busObject);)

#if (0)
    /*
     * Instantiate interfaces. PIO is mandatory, DMA is not.
     * We don't keep interface pointers here because our bus class
     * stores them itself.
     * We do this in SetUpBus because the object must be fully
     * created in order for this stuff to work.
     */
    if (!HIDD_SCSIBus_GetPIOInterface(msg->busObject))
        return FALSE;

    D(bug ("[SCSI:Controller] Hidd_StorageController__SetUpBus: PIO Interfaces obtained\n");)

    if (!SCSIBase->scsi_NoDMA)
        HIDD_SCSIBus_GetDMAInterface(msg->busObject);

    D(bug ("[SCSI:Controller] Hidd_StorageController__SetUpBus: Starting Bus...\n");)
#endif

    /* Add the bus to the device and start service */
    return Hidd_SCSIBus_Start(msg->busObject, SCSIBase);
}

void SCSI__Hidd_StorageController__CleanUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_CleanUpBus *msg)
{
    D(bug ("[SCSI:Controller] Hidd_StorageController__CleanUpBus(0x%p)\n", msg->busObject);)
    /* By default we have nothing to do here */
}
