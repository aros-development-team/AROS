/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction speedbar.gadget - BOOPSI class implementation
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
#include <exec/lists.h>
#include <gadgets/speedbar.h>
#include <utility/tagitem.h>

#include <string.h>

#include "speedbar_intern.h"

#define SpeedBarBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void speedbar_set(Class *cl, Object *o, struct opSet *msg)
{
    struct SpeedBarData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case SPEEDBAR_Buttons:
                data->sd_Buttons = (struct List *)tag->ti_Data;
                break;
            case SPEEDBAR_Orientation:
                data->sd_Orientation = tag->ti_Data;
                break;
            case SPEEDBAR_BevelStyle:
                data->sd_BevelStyle = tag->ti_Data;
                break;
            case SPEEDBAR_Window:
                data->sd_Window = (struct Window *)tag->ti_Data;
                break;
            case SPEEDBAR_EvenSize:
                data->sd_EvenSize = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR SpeedBar__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct SpeedBarData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct SpeedBarData));

        speedbar_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR SpeedBar__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    /* Buttons list is owned by the caller - do not free it here */
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR SpeedBar__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    speedbar_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR SpeedBar__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct SpeedBarData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case SPEEDBAR_Buttons:
            *msg->opg_Storage = (IPTR)data->sd_Buttons;
            return TRUE;

        case SPEEDBAR_Orientation:
            *msg->opg_Storage = data->sd_Orientation;
            return TRUE;

        case SPEEDBAR_BevelStyle:
            *msg->opg_Storage = data->sd_BevelStyle;
            return TRUE;

        case SPEEDBAR_Window:
            *msg->opg_Storage = (IPTR)data->sd_Window;
            return TRUE;

        case SPEEDBAR_EvenSize:
            *msg->opg_Storage = data->sd_EvenSize;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR SpeedBar__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct SpeedBarData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    /* Clear gadget background */
    SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    /* Render buttons from the list */
    if (data->sd_Buttons)
    {
        struct Node *node;
        WORD bx = x + 2;
        WORD by = y + 2;
        UWORD shine  = dri ? dri->dri_Pens[SHINEPEN] : 2;
        UWORD shadow = dri ? dri->dri_Pens[SHADOWPEN] : 1;
        UWORD textpen = dri ? dri->dri_Pens[TEXTPEN] : 1;

        for (node = data->sd_Buttons->lh_Head;
             node->ln_Succ;
             node = node->ln_Succ)
        {
            WORD bw = 32;
            WORD bh = h - 4;

            /* Draw button frame */
            {
                SetAPen(rp, shine);
                Move(rp, bx, by + bh - 1);
                Draw(rp, bx, by);
                Draw(rp, bx + bw - 1, by);

                SetAPen(rp, shadow);
                Move(rp, bx + 1, by + bh - 1);
                Draw(rp, bx + bw - 1, by + bh - 1);
                Draw(rp, bx + bw - 1, by + 1);
            }

            /* Draw button label text if available */
            if (node->ln_Name)
            {
                WORD tx = bx + 2;
                WORD ty = by + rp->TxBaseline + 2;
                WORD maxlen = bw - 4;

                SetAPen(rp, textpen);
                SetDrMd(rp, JAM1);
                Move(rp, tx, ty);

                if (maxlen > 0)
                {
                    ULONG len = strlen(node->ln_Name);
                    Text(rp, node->ln_Name, len);
                }
            }

            /* Advance position for next button */
            if (data->sd_Orientation == 1) /* Vertical */
            {
                by += bh + 1;
                if (by + bh > y + h)
                    break;
            }
            else /* Horizontal (default) */
            {
                bx += bw + 1;
                if (bx + bw > x + w)
                    break;
            }
        }
    }

    /* Disabled ghost pattern overlay */
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

    return TRUE;
}
