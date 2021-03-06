/*
    Copyright (C) 2017, The AROS Development Team. All rights reserved.
*/

#include "gfx_debug.h"

#include <oop/oop.h>
#include <utility/tagitem.h>

#include "gfx_intern.h"

OOP_Object *GFXHW__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    D(bug("[HWGfx] csd @ 0x%p\n", CSD(cl));)

    if (!CSD(cl)->gfxhwinstance)
    {
        struct TagItem new_tags[] =
        {
            {aHW_ClassName, (IPTR)"Display Hardware"},
            {TAG_DONE     , 0          }
        };
        struct pRoot_New new_msg =
        {
            .mID      = msg->mID,
            .attrList = new_tags
        };

        D(bug("[HWGfx] Instantiating...\n");)
        CSD(cl)->gfxhwinstance =  (OOP_Object *)OOP_DoSuperMethod(cl, o, &new_msg.mID);
    }

    D(bug("[HWGfx] returning 0x%p\n", CSD(cl)->gfxhwinstance);)

    return CSD(cl)->gfxhwinstance;
}

VOID GFXHW__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    /* We are singleton. Cannot dispose. */
}
