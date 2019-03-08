/*
    Copyright (C) 2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <hidd/storage.h>
#include <hidd/ahci.h>
#include <hidd/hidd.h>
#include <oop/oop.h>
#include <utility/tagitem.h>
#include <proto/utility.h>

#include "ahci.h"

extern int ahci_attach(device_t dev);
extern void ahci_release(device_t dev);

OOP_Object *AHCI__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct AHCIBase *AHCIBase = (struct AHCIBase *)cl->UserData;

    device_t dev = (device_t)GetTagData(aHidd_DriverData, 0, msg->attrList);

    OOP_Object *ahciController = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (ahciController)
    {
        struct ahci_Controller *data = OOP_INST_DATA(cl, ahciController);
//        D(bug ("[AHCI:Controller] Root__New: New '%s' Controller Obj @ %p\n", new_tags[0].ti_Data, ahciController);)

        /*register the controller in ahci.device */
        D(bug ("[AHCI:Controller] Root__New: Controller Entry @ %p\n", data);)
        D(bug ("[AHCI:Controller] Root__New: Controller Data @ %p\n", dev);)

        data->ac_Class = cl;
        data->ac_Object = ahciController;
        if ((data->ac_dev = dev) != NULL)
            dev->dev_Controller = ahciController;

        AddTail(&AHCIBase->ahci_Controllers, &data->ac_Node);
    }
    return ahciController;
}

VOID AHCI__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct AHCIBase *AHCIBase = cl->UserData;
    struct ahci_Controller *ahciNode, *tmpNode;
    D(bug ("[AHCI:Controller] Root__Dispose(%p)\n", o);)
    ForeachNodeSafe (&AHCIBase->ahci_Controllers, ahciNode, tmpNode)
    {
        if (ahciNode->ac_Object == o)
        {
            D(bug ("[AHCI:Controller] Root__Dispose: Destroying Controller Entry @ %p\n", ahciNode);)
            Remove(&ahciNode->ac_Node);
        }
    }
}

BOOL AHCI__Hidd_StorageController__RemoveBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_RemoveBus *Msg)
{
    D(bug ("[AHCI:Controller] Hidd_StorageController__RemoveBus(%p)\n", o);)
   /*
     * Currently we don't support unloading AHCI bus drivers.
     * This is a very-very big TODO.
     */
    return FALSE;
}

BOOL AHCI__Hidd_StorageController__SetUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_SetUpBus *Msg)
{
    struct AHCIBase *AHCIBase = cl->UserData;
#if (0)
	struct ahci_Controller *data = OOP_INST_DATA(cl, o);
#endif

    D(bug ("[AHCI:Controller] Hidd_StorageController__SetUpBus(%p)\n", Msg->busObject);)

    /* Add the bus to the device and start service */
    return Hidd_AHCIBus_Start(Msg->busObject, AHCIBase);
}

void AHCI__Hidd_StorageController__CleanUpBus(OOP_Class *cl, OOP_Object *o, struct pHidd_StorageController_CleanUpBus *msg)
{
    D(bug ("[AHCI:Controller] Hidd_StorageController__CleanUpBus(%p)\n", o);)
    /* By default we have nothing to do here */
}
