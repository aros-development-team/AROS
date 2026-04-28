/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Scroller gadget class implementation
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
#include <gadgets/scroller.h>
#include <utility/tagitem.h>

#include <string.h>

#include "scroller_intern.h"

#define ScrollerBase ((struct Library *)(cl->cl_UserData))

/* ======================================================================== */

static void scroller_set(Class *cl, Object *o, struct opSet *msg)
{
    struct ScrollerData *data = INST_DATA(cl, o);
    struct TagItem *tag, *taglist = msg->ops_AttrList;

    while ((tag = NextTagItem(&taglist)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case SCROLLER_Total:
            data->total = (ULONG)tag->ti_Data;
            break;

        case SCROLLER_Visible:
            data->visible = (ULONG)tag->ti_Data;
            break;

        case SCROLLER_Top:
            data->top = (ULONG)tag->ti_Data;
            break;

        case SCROLLER_Orientation:
            data->orientation = (ULONG)tag->ti_Data;
            break;

        case SCROLLER_Arrows:
            data->arrows = (BOOL)tag->ti_Data;
            break;

        case SCROLLER_ArrowDelta:
            data->arrowdelta = (ULONG)tag->ti_Data;
            break;
        }
    }

    /* Clamp top so it never exceeds the scrollable range */
    if (data->total > data->visible)
    {
        if (data->top > data->total - data->visible)
            data->top = data->total - data->visible;
    }
    else
    {
        data->top = 0;
    }
}

/* ======================================================================== */

IPTR Scroller__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (!retval)
        return (IPTR)0;

    o = (Object *)retval;

    struct ScrollerData *data = INST_DATA(cl, o);
    memset(data, 0, sizeof(struct ScrollerData));

    /* Sensible defaults */
    data->total       = 1;
    data->visible     = 1;
    data->top         = 0;
    data->orientation = SORIENT_VERT;
    data->arrows      = TRUE;
    data->arrowdelta  = 1;

    scroller_set(cl, o, msg);

    return retval;
}

/* ======================================================================== */

IPTR Scroller__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/* ======================================================================== */

IPTR Scroller__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);

    scroller_set(cl, o, msg);

    return retval;
}

/* ======================================================================== */

IPTR Scroller__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct ScrollerData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
    case SCROLLER_Total:
        *msg->opg_Storage = (IPTR)data->total;
        return (IPTR)TRUE;

    case SCROLLER_Visible:
        *msg->opg_Storage = (IPTR)data->visible;
        return (IPTR)TRUE;

    case SCROLLER_Top:
        *msg->opg_Storage = (IPTR)data->top;
        return (IPTR)TRUE;

    case SCROLLER_Orientation:
        *msg->opg_Storage = (IPTR)data->orientation;
        return (IPTR)TRUE;

    case SCROLLER_Arrows:
        *msg->opg_Storage = (IPTR)data->arrows;
        return (IPTR)TRUE;

    case SCROLLER_ArrowDelta:
        *msg->opg_Storage = (IPTR)data->arrowdelta;
        return (IPTR)TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/* ======================================================================== */

IPTR Scroller__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    IPTR retval;
    struct ScrollerData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct GadgetInfo *gi = msg->gpr_GInfo;

    /* Let PROPGCLASS handle the proportional knob rendering */
    retval = DoSuperMethodA(cl, o, (Msg)msg);

    if (data->arrows && rp && gi)
    {
        WORD x = G(o)->LeftEdge;
        WORD y = G(o)->TopEdge;
        WORD w = G(o)->Width;
        WORD h = G(o)->Height;

        /*
         * Draw thin frames at each end to indicate arrow button regions.
         * A full implementation would render proper arrow imagery here.
         */
        SetAPen(rp, gi->gi_DrInfo ? gi->gi_DrInfo->dri_Pens[SHINEPEN] : 2);

        if (data->orientation == SORIENT_VERT)
        {
            /* Top arrow region frame */
            Move(rp, x, y);
            Draw(rp, x + w - 1, y);
            Draw(rp, x + w - 1, y + w - 1);
            Draw(rp, x, y + w - 1);
            Draw(rp, x, y);

            /* Bottom arrow region frame */
            Move(rp, x, y + h - w);
            Draw(rp, x + w - 1, y + h - w);
            Draw(rp, x + w - 1, y + h - 1);
            Draw(rp, x, y + h - 1);
            Draw(rp, x, y + h - w);
        }
        else
        {
            /* Left arrow region frame */
            Move(rp, x, y);
            Draw(rp, x + h - 1, y);
            Draw(rp, x + h - 1, y + h - 1);
            Draw(rp, x, y + h - 1);
            Draw(rp, x, y);

            /* Right arrow region frame */
            Move(rp, x + w - h, y);
            Draw(rp, x + w - 1, y);
            Draw(rp, x + w - 1, y + h - 1);
            Draw(rp, x + w - h, y + h - 1);
            Draw(rp, x + w - h, y);
        }
    }

    return retval;
}
