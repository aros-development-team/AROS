/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction requester.class - BOOPSI requester class
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/cghooks.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include <classes/requester.h>

#include "requester_intern.h"

/******************************************************************************/

static void requester_set(Class *cl, Object *o, struct TagItem *tags)
{
    struct RequesterData *data = INST_DATA(cl, o);
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case REQ_Type:
                data->rd_Type = tag->ti_Data;
                break;
            case REQ_TitleText:
                data->rd_TitleText = (STRPTR)tag->ti_Data;
                break;
            case REQ_BodyText:
                data->rd_BodyText = (STRPTR)tag->ti_Data;
                break;
            case REQ_GadgetText:
                data->rd_GadgetText = (STRPTR)tag->ti_Data;
                break;
            case REQ_TabSize:
                data->rd_TabSize = tag->ti_Data;
                break;

            /* Integer attributes */
            case REQI_Minimum:
                data->rd_IntMin = (LONG)tag->ti_Data;
                break;
            case REQI_Maximum:
                data->rd_IntMax = (LONG)tag->ti_Data;
                break;
            case REQI_Number:
                data->rd_IntNumber = (LONG)tag->ti_Data;
                break;
            case REQI_Invisible:
                data->rd_IntInvisible = (BOOL)tag->ti_Data;
                break;
            case REQI_Arrows:
                data->rd_IntArrows = (BOOL)tag->ti_Data;
                break;
            case REQI_MaxChars:
                data->rd_IntMaxChars = (UWORD)tag->ti_Data;
                break;

            /* String attributes */
            case REQS_AllowEmpty:
                data->rd_StrAllowEmpty = (BOOL)tag->ti_Data;
                break;
            case REQS_Buffer:
                data->rd_StrBuffer = (UBYTE *)tag->ti_Data;
                break;
            case REQS_ShowDefault:
                data->rd_StrShowDefault = (BOOL)tag->ti_Data;
                break;
            case REQS_MaxChars:
                data->rd_StrMaxChars = tag->ti_Data;
                break;
            case REQS_ChooserArray:
                data->rd_StrChooserArray = (STRPTR *)tag->ti_Data;
                break;
            case REQS_ChooserActive:
                data->rd_StrChooserActive = tag->ti_Data;
                break;

            /* Progress attributes */
            case REQP_Total:
                data->rd_ProgTotal = tag->ti_Data;
                break;
            case REQP_Current:
                data->rd_ProgCurrent = tag->ti_Data;
                break;
            case REQP_OpenInactive:
                data->rd_ProgOpenInactive = (BOOL)tag->ti_Data;
                break;
            case REQP_NoText:
                data->rd_ProgNoText = (BOOL)tag->ti_Data;
                break;
            case REQP_Dynamic:
                data->rd_ProgDynamic = (BOOL)tag->ti_Data;
                break;
            case REQP_CenterWindow:
                data->rd_ProgCenterWin = (struct Window *)tag->ti_Data;
                break;
            case REQP_LastPosition:
                data->rd_ProgLastPos = (BOOL)tag->ti_Data;
                break;
            case REQP_Percent:
                data->rd_ProgPercent = (BOOL)tag->ti_Data;
                break;
            case REQP_Ticks:
                data->rd_ProgTicks = (WORD)tag->ti_Data;
                break;
            case REQP_ShortTicks:
                data->rd_ProgShortTicks = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR Requester__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct RequesterData *data;

    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (!o)
        return (IPTR)NULL;

    data = INST_DATA(cl, o);

    /* Set defaults */
    data->rd_Type           = REQTYPE_INFO;
    data->rd_TitleText      = NULL;
    data->rd_BodyText       = NULL;
    data->rd_GadgetText     = " _Ok | _Cancel ";
    data->rd_ReturnCode     = 0;
    data->rd_TabSize        = 8;
    data->rd_IntMin         = (LONG)0x80000000;
    data->rd_IntMax         = (LONG)0x7FFFFFFF;
    data->rd_IntNumber      = 0;
    data->rd_IntInvisible   = FALSE;
    data->rd_IntArrows      = FALSE;
    data->rd_IntMaxChars    = 10;
    data->rd_StrAllowEmpty  = FALSE;
    data->rd_StrInvisible   = FALSE;
    data->rd_StrBuffer      = NULL;
    data->rd_StrShowDefault = TRUE;
    data->rd_StrMaxChars    = 127;
    data->rd_StrChooserArray = NULL;
    data->rd_StrChooserActive = 0;
    data->rd_ProgTotal      = 100;
    data->rd_ProgCurrent    = 0;
    data->rd_ProgOpenInactive = FALSE;
    data->rd_ProgNoText     = FALSE;
    data->rd_ProgDynamic    = TRUE;
    data->rd_ProgCenterWin  = NULL;
    data->rd_ProgLastPos    = TRUE;
    data->rd_ProgPercent    = FALSE;
    data->rd_ProgTicks      = 0;
    data->rd_ProgShortTicks = FALSE;

    requester_set(cl, o, msg->ops_AttrList);

    return (IPTR)o;
}

/******************************************************************************/

IPTR Requester__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Requester__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    requester_set(cl, o, msg->ops_AttrList);
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Requester__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct RequesterData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case REQ_ReturnCode:
            *msg->opg_Storage = data->rd_ReturnCode;
            return TRUE;
        case REQI_Number:
            *msg->opg_Storage = (IPTR)data->rd_IntNumber;
            return TRUE;
        case REQS_ChooserActive:
            *msg->opg_Storage = data->rd_StrChooserActive;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Requester__RM_OPENREQ(Class *cl, Object *o, struct orRequest *msg)
{
    struct RequesterData *data = INST_DATA(cl, o);
    struct Window *refwin = msg->or_Window;
    struct Screen *refscr = msg->or_Screen;
    LONG result = 0;

    /* Apply any additional tags from the method */
    if (msg->or_Attrs)
        requester_set(cl, o, msg->or_Attrs);

    /* If no reference window and no screen, try to get the default public screen */
    if (!refwin && !refscr)
        return 0;

    /* Use the window's screen if no explicit screen given */
    if (refwin && !refscr)
        refscr = refwin->WScreen;

    switch (data->rd_Type)
    {
        case REQTYPE_INFO:
        {
            struct EasyStruct es;

            es.es_StructSize   = sizeof(struct EasyStruct);
            es.es_Flags        = 0;
            es.es_Title        = data->rd_TitleText ? data->rd_TitleText : (STRPTR)"Request";
            es.es_TextFormat   = data->rd_BodyText ? data->rd_BodyText : (STRPTR)"";
            es.es_GadgetFormat = data->rd_GadgetText ? data->rd_GadgetText : (STRPTR)" _Ok ";

            result = EasyRequestArgs(refwin, &es, NULL, NULL);
            break;
        }

        case REQTYPE_INTEGER:
        case REQTYPE_STRING:
        case REQTYPE_PROGRESS:
            /* These require more complex UI building.
               For now, fall through to the simple requester.
               Full implementation would create layout objects. */
            {
                struct EasyStruct es;

                es.es_StructSize   = sizeof(struct EasyStruct);
                es.es_Flags        = 0;
                es.es_Title        = data->rd_TitleText ? data->rd_TitleText : (STRPTR)"Request";
                es.es_TextFormat   = data->rd_BodyText ? data->rd_BodyText : (STRPTR)"";
                es.es_GadgetFormat = data->rd_GadgetText ? data->rd_GadgetText : (STRPTR)" _Ok | _Cancel ";

                result = EasyRequestArgs(refwin, &es, NULL, NULL);
            }
            break;
    }

    data->rd_ReturnCode = result;
    return (IPTR)result;
}
