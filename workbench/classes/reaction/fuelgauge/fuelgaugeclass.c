/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction fuelgauge.gadget - BOOPSI class implementation
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
#include <gadgets/fuelgauge.h>
#include <utility/tagitem.h>

#include <string.h>

#include "fuelgauge_intern.h"

#define FuelGaugeBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void fuelgauge_set(Class *cl, Object *o, struct opSet *msg)
{
    struct FuelGaugeData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case FUELGAUGE_Level:
                data->fgd_Level = (LONG)tag->ti_Data;
                break;
            case FUELGAUGE_Min:
                data->fgd_Min = (LONG)tag->ti_Data;
                break;
            case FUELGAUGE_Max:
                data->fgd_Max = (LONG)tag->ti_Data;
                break;
            case FUELGAUGE_Orientation:
                data->fgd_Orientation = tag->ti_Data;
                break;
            case FUELGAUGE_Ticks:
                data->fgd_Ticks = (UWORD)tag->ti_Data;
                break;
            case FUELGAUGE_ShortTicks:
                data->fgd_ShortTicks = (UWORD)tag->ti_Data;
                break;
            case FUELGAUGE_Percent:
                data->fgd_Percent = (BOOL)tag->ti_Data;
                break;
            case FUELGAUGE_Justification:
                data->fgd_Justification = tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR FuelGauge__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct FuelGaugeData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct FuelGaugeData));
        data->fgd_Min         = 0;
        data->fgd_Max         = 100;
        data->fgd_Level       = 0;
        data->fgd_Orientation = FUELGAUGE_ORIENT_HORIZ;
        data->fgd_Percent     = TRUE;

        fuelgauge_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR FuelGauge__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR FuelGauge__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    fuelgauge_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR FuelGauge__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct FuelGaugeData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case FUELGAUGE_Level:
            *msg->opg_Storage = data->fgd_Level;
            return TRUE;

        case FUELGAUGE_Min:
            *msg->opg_Storage = data->fgd_Min;
            return TRUE;

        case FUELGAUGE_Max:
            *msg->opg_Storage = data->fgd_Max;
            return TRUE;

        case FUELGAUGE_Orientation:
            *msg->opg_Storage = data->fgd_Orientation;
            return TRUE;

        case FUELGAUGE_Ticks:
            *msg->opg_Storage = data->fgd_Ticks;
            return TRUE;

        case FUELGAUGE_ShortTicks:
            *msg->opg_Storage = data->fgd_ShortTicks;
            return TRUE;

        case FUELGAUGE_Percent:
            *msg->opg_Storage = data->fgd_Percent;
            return TRUE;

        case FUELGAUGE_Justification:
            *msg->opg_Storage = data->fgd_Justification;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR FuelGauge__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct FuelGaugeData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    LONG range, fillSize;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    /* Compute the filled portion */
    range = data->fgd_Max - data->fgd_Min;
    if (range <= 0)
        range = 1;

    /* Clamp level to valid range */
    LONG level = data->fgd_Level;
    if (level < data->fgd_Min)
        level = data->fgd_Min;
    if (level > data->fgd_Max)
        level = data->fgd_Max;

    /* Draw background */
    if (dri)
        SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    else
        SetAPen(rp, 0);

    RectFill(rp, x, y, x + w - 1, y + h - 1);

    /* Draw filled portion */
    if (dri)
        SetAPen(rp, dri->dri_Pens[FILLPEN]);
    else
        SetAPen(rp, 3);

    if (data->fgd_Orientation == FUELGAUGE_ORIENT_HORIZ)
    {
        fillSize = (w * (level - data->fgd_Min)) / range;
        if (fillSize > 0)
            RectFill(rp, x, y, x + fillSize - 1, y + h - 1);
    }
    else
    {
        fillSize = (h * (level - data->fgd_Min)) / range;
        if (fillSize > 0)
            RectFill(rp, x, y + h - fillSize, x + w - 1, y + h - 1);
    }

    /* Draw percentage text if enabled */
    if (data->fgd_Percent)
    {
        char buf[8];
        LONG pct = ((level - data->fgd_Min) * 100) / range;
        WORD len, txWidth, txX, txY;

        if (pct >= 100)
        {
            buf[0] = '1'; buf[1] = '0'; buf[2] = '0'; buf[3] = '%'; buf[4] = '\0';
        }
        else if (pct >= 10)
        {
            buf[0] = '0' + (pct / 10);
            buf[1] = '0' + (pct % 10);
            buf[2] = '%'; buf[3] = '\0';
        }
        else
        {
            buf[0] = '0' + pct;
            buf[1] = '%'; buf[2] = '\0';
        }

        len = strlen(buf);
        txWidth = TextLength(rp, buf, len);

        if (dri)
            SetAPen(rp, dri->dri_Pens[TEXTPEN]);
        else
            SetAPen(rp, 1);

        SetDrMd(rp, JAM1);

        txX = x + (w - txWidth) / 2;
        txY = y + (h - rp->TxHeight) / 2 + rp->TxBaseline;

        Move(rp, txX, txY);
        Text(rp, buf, len);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}
