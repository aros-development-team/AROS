/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
    $Id: kbdsubsystem.c 46449 2013-02-02 15:08:09Z sonic $
*/

#include <aros/debug.h>
#include <hidd/ata.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include "ata.h"

OOP_Object *ATA__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct ataBase *ATABase = cl->UserData;

    if (!ATABase->ataObj)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"IDE"},
            {TAG_DONE     , 0          }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        ATABase->ataObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }
    return ATABase->ataObj;
}

VOID ATA__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* We are singletone. Cannot dispose. */
}

BOOL ATA__HW__RemoveDriver(OOP_Class *cl, OOP_Object *o, struct pHW_RemoveDriver *Msg)
{
    /*
     * Currently we don't support unloading bus drivers.
     * This is a very-very big TODO.
     */
    return FALSE;
}

BOOL ATA__HW__SetupDriver(OOP_Class *cl, OOP_Object *o, struct pHW_SetupDriver *Msg)
{
    struct ataBase *ATABase = cl->UserData;

    /*
     * Instantiate interfaces. PIO is mandatory, DMA is not.
     * We don't keep interface pointers here because our bus class
     * stores them itself.
     * We do this in SetupDriver because the object must be fully
     * created in order for this stuff to work.
     */
    if (!HIDD_ATABus_GetPIOInterface(Msg->driverObject))
        return FALSE;

    if (!ATABase->ata_NoDMA)
        HIDD_ATABus_GetDMAInterface(Msg->driverObject);

    /* Add the bus to the device and start service */
    return Hidd_ATABus_Start(Msg->driverObject, ATABase);
}
