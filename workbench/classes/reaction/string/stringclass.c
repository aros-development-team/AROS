/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction string.gadget - BOOPSI class implementation
*/

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

#define StringGadBase ((struct Library *)(cl->cl_UserData))

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
            case STRINGA_MaxChars:
                data->sd_MaxChars = tag->ti_Data;
                break;
            case STRINGA_Buffer:
                data->sd_Buffer = (STRPTR)tag->ti_Data;
                break;
            case STRINGA_BufferPos:
                data->sd_BufferPos = tag->ti_Data;
                break;
            case STRINGA_DispPos:
                data->sd_DispPos = tag->ti_Data;
                break;
            case STRINGA_Justification:
                data->sd_Justification = tag->ti_Data;
                break;
            case STRINGA_EditHook:
                data->sd_EditHook = (struct Hook *)tag->ti_Data;
                break;
            case STRINGA_ReplaceMode:
                data->sd_ReplaceMode = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR StringGad__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct StringGadData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct StringGadData));

        /* Set default values */
        data->sd_MaxChars = 256;

        string_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR StringGad__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR StringGad__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    string_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR StringGad__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct StringGadData *data = INST_DATA(cl, o);
    struct StringInfo *si = (struct StringInfo *)G(o)->SpecialInfo;

    switch (msg->opg_AttrID)
    {
        case STRINGA_MaxChars:
            *msg->opg_Storage = data->sd_MaxChars;
            return TRUE;

        case STRINGA_Buffer:
            *msg->opg_Storage = (IPTR)data->sd_Buffer;
            return TRUE;

        case STRINGA_TextVal:
            *msg->opg_Storage = si ? (IPTR)si->Buffer : (IPTR)NULL;
            return TRUE;

        case STRINGA_LongVal:
            *msg->opg_Storage = si ? si->LongInt : 0;
            return TRUE;

        case STRINGA_Justification:
            *msg->opg_Storage = data->sd_Justification;
            return TRUE;

        case STRINGA_BufferPos:
            *msg->opg_Storage = data->sd_BufferPos;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR StringGad__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    /* STRGCLASS handles all string rendering */
    return DoSuperMethodA(cl, o, (Msg)msg);
}
