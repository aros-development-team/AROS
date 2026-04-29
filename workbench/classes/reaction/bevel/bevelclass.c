/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bevel.image - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <images/bevel.h>
#include <utility/tagitem.h>

#include <string.h>

#include "bevel_intern.h"

#define BevelBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void bevel_set(Class *cl, Object *o, struct opSet *msg)
{
    struct BevelData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case BEVEL_Style:
                data->bd_Style = tag->ti_Data;
                break;
            case BEVEL_Label:
                data->bd_Label = (STRPTR)tag->ti_Data;
                break;
            case BEVEL_LabelPlace:
                data->bd_LabelPlace = tag->ti_Data;
                break;
            case BEVEL_TextPen:
                data->bd_TextPen = (UWORD)tag->ti_Data;
                break;
            case BEVEL_FillPen:
                data->bd_FillPen = (UWORD)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

static void bevel_draw_thin(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, pens[SHADOWPEN]);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x, y + h - 1);
}

static void bevel_draw_button(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h, ULONG state)
{
    UWORD shinePen, shadowPen;

    if (state == IDS_SELECTED || state == IDS_INACTIVESELECTED)
    {
        shinePen = pens[SHADOWPEN];
        shadowPen = pens[SHINEPEN];
    }
    else
    {
        shinePen = pens[SHINEPEN];
        shadowPen = pens[SHADOWPEN];
    }

    /* Outer top-left highlight */
    SetAPen(rp, shinePen);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    /* Outer bottom-right shadow */
    SetAPen(rp, shadowPen);
    Move(rp, x + w - 1, y + 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + 1, y + h - 1);

    /* Inner top-left highlight */
    SetAPen(rp, shinePen);
    Move(rp, x + 1, y + h - 2);
    Draw(rp, x + 1, y + 1);
    Draw(rp, x + w - 2, y + 1);

    /* Inner bottom-right shadow */
    SetAPen(rp, shadowPen);
    Move(rp, x + w - 2, y + 2);
    Draw(rp, x + w - 2, y + h - 2);
    Draw(rp, x + 2, y + h - 2);
}

static void bevel_draw_group(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    /* Recessed outer frame: shadow on top-left, shine on bottom-right */
    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x, y + h - 2);
    Draw(rp, x, y);
    Draw(rp, x + w - 2, y);

    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x + w - 1, y);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x, y + h - 1);

    /* Inner highlight */
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x + 1, y + h - 3);
    Draw(rp, x + 1, y + 1);
    Draw(rp, x + w - 3, y + 1);

    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x + w - 2, y + 1);
    Draw(rp, x + w - 2, y + h - 2);
    Draw(rp, x + 1, y + h - 2);
}

static void bevel_draw_field(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    /* Recessed field: dark top-left, bright bottom-right */
    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x + w - 1, y + 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + 1, y + h - 1);
}

static void bevel_draw_box(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    /* Raised box: shine top-left, shadow bottom-right */
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x + w - 1, y + 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + 1, y + h - 1);
}

/******************************************************************************/

IPTR Bevel__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct BevelData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct BevelData));
        data->bd_Style = BVS_GROUP;

        bevel_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Bevel__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Bevel__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    bevel_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Bevel__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct BevelData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case BEVEL_Style:
            *msg->opg_Storage = data->bd_Style;
            return TRUE;

        case BEVEL_Label:
            *msg->opg_Storage = (IPTR)data->bd_Label;
            return TRUE;

        case BEVEL_LabelPlace:
            *msg->opg_Storage = data->bd_LabelPlace;
            return TRUE;

        case BEVEL_TextPen:
            *msg->opg_Storage = data->bd_TextPen;
            return TRUE;

        case BEVEL_FillPen:
            *msg->opg_Storage = data->bd_FillPen;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Bevel__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct BevelData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawInfo *dri = msg->imp_DrInfo;
    UWORD *pens;
    WORD x, y, w, h;

    if (!rp || !dri)
        return FALSE;

    pens = dri->dri_Pens;
    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = im->Width;
    h = im->Height;

    if (w < 2 || h < 2)
        return TRUE;

    switch (data->bd_Style)
    {
        case BVS_NONE:
            break;

        case BVS_THIN:
            bevel_draw_thin(rp, pens, x, y, w, h);
            break;

        case BVS_BUTTON:
            bevel_draw_button(rp, pens, x, y, w, h, msg->imp_State);
            break;

        case BVS_GROUP:
            bevel_draw_group(rp, pens, x, y, w, h);
            break;

        case BVS_FIELD:
            bevel_draw_field(rp, pens, x, y, w, h);
            break;

        case BVS_BOX:
            bevel_draw_box(rp, pens, x, y, w, h);
            break;

        case BVS_DROPBOX:
        case BVS_SBAR_VERT:
        case BVS_SBAR_HORIZ:
        default:
            bevel_draw_group(rp, pens, x, y, w, h);
            break;
    }

    /* Draw label if present (for group-style bevels) */
    if (data->bd_Label && data->bd_Style == BVS_GROUP)
    {
        UWORD pen = data->bd_TextPen ? data->bd_TextPen : pens[TEXTPEN];
        WORD textX = x + 8;
        WORD textY = y;
        ULONG len = strlen(data->bd_Label);

        SetAPen(rp, pens[BACKGROUNDPEN]);
        RectFill(rp, textX - 2, textY - 1, textX + TextLength(rp, data->bd_Label, len) + 1, textY + rp->TxHeight - 1);

        SetAPen(rp, pen);
        SetDrMd(rp, JAM1);
        Move(rp, textX, textY + rp->TxBaseline);
        Text(rp, data->bd_Label, len);
    }

    return TRUE;
}
