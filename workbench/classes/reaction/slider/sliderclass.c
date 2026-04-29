/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction slider.gadget - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <gadgets/slider.h>
#include <utility/tagitem.h>

#include <string.h>

#include "slider_intern.h"

#define SliderBase ((struct Library *)(cl->cl_UserData))

#define TICK_LENGTH     6
#define SHORTTICK_LENGTH 3

/******************************************************************************/

static void slider_set(Class *cl, Object *o, struct opSet *msg)
{
    struct SliderData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case SLIDER_Min:
                data->sd_Min = (LONG)tag->ti_Data;
                break;
            case SLIDER_Max:
                data->sd_Max = (LONG)tag->ti_Data;
                break;
            case SLIDER_Level:
                data->sd_Level = (LONG)tag->ti_Data;
                break;
            case SLIDER_Orientation:
                data->sd_Orientation = tag->ti_Data;
                break;
            case SLIDER_Ticks:
                data->sd_Ticks = (UWORD)tag->ti_Data;
                break;
            case SLIDER_ShortTicks:
                data->sd_ShortTicks = (UWORD)tag->ti_Data;
                break;
            case SLIDER_Invert:
                data->sd_Invert = (BOOL)tag->ti_Data;
                break;
        }
    }

    /* Clamp level to valid range */
    if (data->sd_Level < data->sd_Min)
        data->sd_Level = data->sd_Min;
    if (data->sd_Level > data->sd_Max)
        data->sd_Level = data->sd_Max;
}

/******************************************************************************/

IPTR Slider__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct SliderData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct SliderData));

        /* Defaults */
        data->sd_Min         = 0;
        data->sd_Max         = 100;
        data->sd_Level       = 0;
        data->sd_Orientation = SLIDER_HORIZONTAL;

        slider_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Slider__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Slider__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    slider_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Slider__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct SliderData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case SLIDER_Min:
            *msg->opg_Storage = data->sd_Min;
            return TRUE;

        case SLIDER_Max:
            *msg->opg_Storage = data->sd_Max;
            return TRUE;

        case SLIDER_Level:
            *msg->opg_Storage = data->sd_Level;
            return TRUE;

        case SLIDER_Orientation:
            *msg->opg_Storage = data->sd_Orientation;
            return TRUE;

        case SLIDER_Ticks:
            *msg->opg_Storage = data->sd_Ticks;
            return TRUE;

        case SLIDER_ShortTicks:
            *msg->opg_Storage = data->sd_ShortTicks;
            return TRUE;

        case SLIDER_Invert:
            *msg->opg_Storage = data->sd_Invert;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

static void slider_draw_ticks(struct RastPort *rp, struct DrawInfo *dri,
                              struct SliderData *data,
                              WORD x, WORD y, WORD w, WORD h)
{
    UWORD pen = dri ? dri->dri_Pens[SHADOWPEN] : 1;
    UWORD total_ticks;
    WORD i;

    if (data->sd_Ticks < 2)
        return;

    total_ticks = data->sd_Ticks;
    if (data->sd_ShortTicks > 0)
        total_ticks = (data->sd_Ticks - 1) * (data->sd_ShortTicks + 1) + 1;

    SetAPen(rp, pen);

    for (i = 0; i < total_ticks; i++)
    {
        BOOL is_major;
        WORD tick_len;
        WORD pos;

        if (data->sd_ShortTicks > 0)
            is_major = (i % (data->sd_ShortTicks + 1)) == 0;
        else
            is_major = TRUE;

        tick_len = is_major ? TICK_LENGTH : SHORTTICK_LENGTH;

        if (data->sd_Orientation == SLIDER_VERTICAL)
        {
            pos = y + (i * (h - 1)) / (total_ticks - 1);
            Move(rp, x + w, pos);
            Draw(rp, x + w + tick_len - 1, pos);
        }
        else
        {
            pos = x + (i * (w - 1)) / (total_ticks - 1);
            Move(rp, pos, y + h);
            Draw(rp, pos, y + h + tick_len - 1);
        }
    }
}

/******************************************************************************/

IPTR Slider__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct SliderData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    IPTR retval;
    WORD x, y, w, h;

    /* Let PROPGCLASS handle the proportional knob rendering */
    retval = DoSuperMethodA(cl, o, (Msg)msg);

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return retval;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    /* Draw tick marks */
    if (data->sd_Ticks > 0)
    {
        slider_draw_ticks(rp, dri, data, x, y, w, h);
    }

    /* Disabled rendering */
    if (gad->Flags & GFLG_DISABLED)
    {
        ULONG pattern[] = { 0x88888888, 0x22222222 };
        SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
        SetAfPt(rp, (UWORD *)pattern, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return retval;
}
