/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction string.gadget - BOOPSI class implementation
*/
#define DEBUG 1

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <gadgets/string.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <string.h>

#include "string_intern.h"

#define StringBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void string_set(Class *cl, Object *o, struct opSet *msg)
{
    struct StringGadData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case STRINGA_MinVisible:
                data->sd_MinVisible = (UWORD)tag->ti_Data;
                break;
            case STRINGA_HookType:
                data->sd_HookType = (UWORD)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR String__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[String] OM_NEW: enter\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    D(bug("[String] OM_NEW: obj=%p\n", (void *)retval));
    if (retval)
    {
        struct StringGadData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct StringGadData));

        string_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR String__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[String] OM_DISPOSE: obj=%p\n", (void *)o));

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR String__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[String] OM_SET: obj=%p\n", (void *)o));

    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    string_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR String__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct StringGadData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case STRINGA_MinVisible:
            *msg->opg_Storage = data->sd_MinVisible;
            return TRUE;

        case STRINGA_HookType:
            *msg->opg_Storage = data->sd_HookType;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR String__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    D(bug("[String] GM_RENDER: obj=%p redraw=%ld\n", (void *)o, msg->gpr_Redraw));

    /* STRGCLASS handles all string rendering */
    return DoSuperMethodA(cl, o, (Msg)msg);
}
