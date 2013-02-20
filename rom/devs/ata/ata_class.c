/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
    $Id: kbdsubsystem.c 46449 2013-02-02 15:08:09Z sonic $
*/

#include <aros/debug.h>
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
            {aHW_ClassName, (IPTR)"ATA"},
            {TAG_DONE     , 0                }
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

}

OOP_Object *ATA__HW__AddDriver(OOP_Class *cl, OOP_Object *o, struct pHW_AddDriver *Msg)
{
    struct TagItem tags[] =
    {
        {TAG_MORE, (IPTR)Msg->tags}
    };
    struct pHW_AddDriver add_msg =
    {
        .mID         = Msg->mID,
        .driverClass = Msg->driverClass,
        .tags        = tags
    };

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &add_msg.mID);
}
