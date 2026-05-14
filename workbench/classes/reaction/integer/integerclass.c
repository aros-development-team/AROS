/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction integer.gadget - BOOPSI class implementation
*/
#define DEBUG 1

#include <string.h>
#include <limits.h>

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <gadgets/integer.h>

#include "integer_intern.h"

#define IntegerBase ((struct Library *)(cl->cl_UserData))

/* ------------------------------------------------------------------ */

static void integer_set(Class *cl, Object *o, struct opSet *msg)
{
    struct IntegerData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case INTEGER_Number:
                data->number = (LONG)tag->ti_Data;
                if (data->number < data->minimum)
                    data->number = data->minimum;
                if (data->number > data->maximum)
                    data->number = data->maximum;
                break;

            case INTEGER_Minimum:
                data->minimum = (LONG)tag->ti_Data;
                break;

            case INTEGER_Maximum:
                data->maximum = (LONG)tag->ti_Data;
                break;

            case INTEGER_MaxChars:
                data->maxchars = (ULONG)tag->ti_Data;
                break;

            case INTEGER_Arrows:
                data->arrows = (BOOL)tag->ti_Data;
                break;
        }
    }

    /* Clamp to range after min/max may have changed */
    if (data->number < data->minimum)
        data->number = data->minimum;
    if (data->number > data->maximum)
        data->number = data->maximum;
}

/* ------------------------------------------------------------------ */

IPTR Integer__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct IntegerData *data;

    D(bug("[Integer] OM_NEW: enter\n"));

    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    D(bug("[Integer] OM_NEW: obj=%p\n", (void *)o));
    if (!o)
        return (IPTR)0;

    data = INST_DATA(cl, o);
    memset(data, 0, sizeof(struct IntegerData));

    /* Defaults */
    data->minimum      = (LONG)0x80000000;
    data->maximum      = (LONG)0x7FFFFFFF;
    data->maxchars     = 10;

    integer_set(cl, o, msg);

    return (IPTR)o;
}

/* ------------------------------------------------------------------ */

IPTR Integer__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[Integer] OM_DISPOSE: obj=%p\n", (void *)o));

    return DoSuperMethodA(cl, o, msg);
}

/* ------------------------------------------------------------------ */

IPTR Integer__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[Integer] OM_SET: obj=%p\n", (void *)o));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    integer_set(cl, o, msg);

    return retval;
}

/* ------------------------------------------------------------------ */

IPTR Integer__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct IntegerData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case INTEGER_Number:
            *msg->opg_Storage = (IPTR)data->number;
            return (IPTR)TRUE;

        case INTEGER_Minimum:
            *msg->opg_Storage = (IPTR)data->minimum;
            return (IPTR)TRUE;

        case INTEGER_Maximum:
            *msg->opg_Storage = (IPTR)data->maximum;
            return (IPTR)TRUE;

        case INTEGER_MaxChars:
            *msg->opg_Storage = (IPTR)data->maxchars;
            return (IPTR)TRUE;

        case INTEGER_Arrows:
            *msg->opg_Storage = (IPTR)data->arrows;
            return (IPTR)TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/* ------------------------------------------------------------------ */

IPTR Integer__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    IPTR retval;

    D(bug("[Integer] GM_RENDER: obj=%p redraw=%ld\n", (void *)o, msg->gpr_Redraw));

    /* Let the STRGCLASS superclass render the string area */
    retval = DoSuperMethodA(cl, o, (Msg)msg);

    /* TODO: draw frame and optional inc/dec arrows around string area */

    return retval;
}
