/*
    Copyright (C) 2015-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "storage_intern.h"

OOP_Object *StorageHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug ("[Storage] Root__New()\n");)
    if (!CSD(cl)->instance)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"Storage Controllers"},
            {TAG_DONE     , 0          }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        CSD(cl)->instance =  (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }

    D(bug ("[Storage] Root__New: Instance @ 0x%p\n", CSD(cl)->instance);)
    return CSD(cl)->instance;
}

VOID StorageHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug ("[Storage] Root__Dispose(0x%p)\n", o);)
    /* We are singletone. Cannot dispose. */
}


VOID StorageHW__Hidd_Storage__AllocateID(OOP_Class *cl, OOP_Object *o, struct pHidd_Storage_AllocateID *msg)
{
    D(bug ("[Storage] Hidd_Storage__AllocateID(0x%p)\n", o);)
}

VOID StorageHW__Hidd_Storage__ReleaseID(OOP_Class *cl, OOP_Object *o, struct pHidd_Storage_ReleaseID *msg)
{
    D(bug ("[Storage] Hidd_Storage__ReleaseID(0x%p)\n", o);)
}
