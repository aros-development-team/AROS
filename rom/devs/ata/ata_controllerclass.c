/*
    Copyright (C) 2013-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/utility.h>

#include <hidd/storage.h>
#include <hidd/ata.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "ata.h"

const char ata_IDEName[] = "IDE Controller";

OOP_Object *ATA__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct ataBase *ATABase = cl->UserData;
//	char *ataControllerName = (char *)GetTagData(aHidd_HardwareName, (IPTR)ata_IDEName, msg->attrList);

    OOP_Object *ataController = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (ataController)
    {
        struct ata_Controller *data = OOP_INST_DATA(cl, ataController);
//		data->ac_Node.ln_Name = ataControllerName;

//		D(bug ("[ATA:Controller] Root__New: New '%s' Controller Obj @ 0x%p\n", ataControllerName, ataController);)

        /*register the controller in ata.device */
        D(bug ("[ATA:Controller] %s: Controller Entry @ 0x%p\n", __func__, data);)

        data->ac_Class = cl;
        data->ac_Object = ataController;

        /* Try to setup daemon task looking for diskchanges */
        NEWLIST(&data->Daemon_ios);
        InitSemaphore(&data->DaemonSem);
        data->ac_daemonParent = FindTask(NULL);
        SetSignal(0, SIGF_SINGLE);

        if (!NewCreateTask(TASKTAG_PC, DaemonCode,
                           TASKTAG_NAME       , "ATA.daemon",
                           TASKTAG_STACKSIZE  , STACK_SIZE,
                           TASKTAG_TASKMSGPORT, &data->DaemonPort,
                           TASKTAG_PRI        , TASK_PRI - 1,	/* The daemon should have a little bit lower Pri than handler tasks */
                           TASKTAG_ARG1       , ATABase,
                           TASKTAG_ARG2       , data,
                           TAG_DONE))
        {
            bug("[ATA:Controller] %s: Failed to start up daemon!\n", __func__);
            return FALSE;
        }

        /* Wait for handshake */
        Wait(SIGF_SINGLE);
        D(bug("[ATA:Controller] %s: Daemon task set to 0x%p\n", __func__, data->ata_Daemon));

        AddTail(&ATABase->ata_Controllers, &data->ac_Node);
    }
    return ataController;
}

VOID ATA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct ataBase *ATABase = cl->UserData;
    struct ata_Controller *ataNode, *tmpNode;

    D(bug ("[ATA:Controller] %s(0x%p)\n", o);)

    ForeachNodeSafe (&ATABase->ata_Controllers, ataNode, tmpNode)
    {
        if (ataNode->ac_Object == o)
        {
            D(bug("[ATA:Controller] %s: Stopping Daemon...\n", __func__));
            ataNode->ac_daemonParent = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            Signal(ataNode->ac_Daemon, SIGBREAKF_CTRL_C);
            Wait(SIGF_SINGLE);
            D(bug("[ATA:Controller] %s: Daemon stopped\n", __func__));

            D(bug ("[ATA:Controller] %s: Destroying Controller Entry @ 0x%p\n", __func__, ataNode);)
            Remove(&ataNode->ac_Node);

            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
            return;
        }
    }
}

void ATA__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
#if (0)
    struct ata_Controller *data = OOP_INST_DATA(cl, o);
    IPTR idx;

    if (IS_ATA_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_HardwareName:
            *msg->storage = (IPTR)data->ac_Node.ln_Name;
            return;
        }
    }
#endif
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

BOOL ATA__Hidd_StorageController__RemoveBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_RemoveBus *msg)
{
    D(bug ("[ATA:Controller] Hidd_StorageController__RemoveBus(0x%p)\n", msg->busObject);)
   /*
     * Currently we don't support unloading ATA bus drivers.
     * This is a very-very big TODO.
     */
    return FALSE;
}

BOOL ATA__Hidd_StorageController__SetUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_SetUpBus *msg)
{
    struct ataBase *ATABase = cl->UserData;
    struct TagItem busTags[2];

    D(bug ("[ATA:Controller] Hidd_StorageController__SetUpBus(0x%p)\n", msg->busObject);)

    /*
     * Instantiate interfaces. PIO is mandatory, DMA is not.
     * We don't keep interface pointers here because our bus class
     * stores them itself.
     * We do this in SetUpBus because the object must be fully
     * created in order for this stuff to work.
     */
    if (!HIDD_ATABus_GetPIOInterface(msg->busObject))
        return FALSE;

    D(bug ("[ATA:Controller] Hidd_StorageController__SetUpBus: PIO Interfaces obtained\n");)

    if (!ATABase->ata_NoDMA)
        HIDD_ATABus_GetDMAInterface(msg->busObject);

    busTags[0].ti_Tag = aHidd_ATABus_Controller;
    busTags[0].ti_Data = (IPTR)OOP_INST_DATA(cl, o);
    busTags[1].ti_Tag = TAG_DONE;
    OOP_SetAttrs(msg->busObject, busTags);

    D(bug ("[ATA:Controller] Hidd_StorageController__SetUpBus: Starting Bus...\n");)

    /* Add the bus to the device and start service */
    return Hidd_ATABus_Start(msg->busObject, ATABase);
}

void ATA__Hidd_StorageController__CleanUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_CleanUpBus *msg)
{
    D(bug ("[ATA:Controller] Hidd_StorageController__CleanUpBus(0x%p)\n", msg->busObject);)
    /* By default we have nothing to do here */
}
