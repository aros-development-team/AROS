/*
    Copyright (C) 2015-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "system_intern.h"

IPTR SystemHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    if (!CSD(cl)->instance)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"System Devices"},
            {TAG_DONE     , 0          }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        CSD(cl)->instance =  (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }

    return (IPTR)CSD(cl)->instance;
}

VOID SystemHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* We are singletone. Cannot dispose. */
}
