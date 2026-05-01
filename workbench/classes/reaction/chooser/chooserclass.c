/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction chooser.gadget - BOOPSI class implementation
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
#include <gadgets/chooser.h>
#include <utility/tagitem.h>

#include <string.h>

#include "chooser_intern.h"

#define ChooserBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static LONG chooser_count_labels(struct List *labels)
{
    LONG count = 0;

    if (labels)
    {
        struct MinNode *node;
        for (node = (struct MinNode *)labels->lh_Head;
             node->mln_Succ;
             node = node->mln_Succ)
        {
            count++;
        }
    }
    return count;
}

static struct ChooserNode *chooser_get_node(struct List *labels, LONG index)
{
    if (labels)
    {
        struct MinNode *node;
        LONG i = 0;
        for (node = (struct MinNode *)labels->lh_Head;
             node->mln_Succ;
             node = node->mln_Succ, i++)
        {
            if (i == index)
                return (struct ChooserNode *)node;
        }
    }
    return NULL;
}

/******************************************************************************/

static void chooser_set(Class *cl, Object *o, struct opSet *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case CHOOSER_Labels:
                data->cd_Labels = (struct List *)tag->ti_Data;
                data->cd_NumLabels = chooser_count_labels(data->cd_Labels);
                break;
            case CHOOSER_Active:
                data->cd_Selected = (LONG)tag->ti_Data;
                break;
            case CHOOSER_MaxLabels:
                data->cd_MaxLabels = (LONG)tag->ti_Data;
                break;
            case CHOOSER_DropDown:
                data->cd_DropDown = (BOOL)tag->ti_Data;
                break;
            case CHOOSER_AutoFit:
                data->cd_AutoFit = (BOOL)tag->ti_Data;
                break;
            case CHOOSER_Title:
                data->cd_Title = (STRPTR)tag->ti_Data;
                break;
            case CHOOSER_PopUp:
                data->cd_PopUp = (BOOL)tag->ti_Data;
                break;
            case CHOOSER_Hidden:
                data->cd_Hidden = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR Chooser__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct ChooserData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct ChooserData));
        data->cd_Selected = 0;
        data->cd_DropDown = TRUE;

        chooser_set(cl, (Object *)retval, msg);

        /* Handle CHOOSER_LabelArray shortcut */
        {
            struct TagItem *tag;
            tag = FindTagItem(CHOOSER_LabelArray, msg->ops_AttrList);
            if (tag && tag->ti_Data && !data->cd_Labels)
            {
                /* LabelArray is handled at the application level;
                   the caller should convert to ChooserNode list */
            }
        }
    }

    return retval;
}

/******************************************************************************/

IPTR Chooser__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Chooser__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    chooser_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Chooser__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case CHOOSER_Active:
            *msg->opg_Storage = data->cd_Selected;
            return TRUE;

        case CHOOSER_Labels:
            *msg->opg_Storage = (IPTR)data->cd_Labels;
            return TRUE;

        case CHOOSER_DropDown:
            *msg->opg_Storage = data->cd_DropDown;
            return TRUE;

        case CHOOSER_Title:
            *msg->opg_Storage = (IPTR)data->cd_Title;
            return TRUE;

        case CHOOSER_MaxLabels:
            *msg->opg_Storage = data->cd_MaxLabels;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

static void chooser_draw_arrow(struct RastPort *rp, WORD x, WORD y,
                               WORD w, WORD h, UWORD pen)
{
    WORD cx = x + w / 2;
    WORD ay = y + h / 3;
    WORD by = y + (h * 2) / 3;

    SetAPen(rp, pen);
    Move(rp, cx - 4, ay);
    Draw(rp, cx, by);
    Draw(rp, cx + 4, ay);
}

IPTR Chooser__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    UWORD shine, shadow, bg, text_pen;
    STRPTR label = NULL;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    shine   = dri ? dri->dri_Pens[SHINEPEN]       : 2;
    shadow  = dri ? dri->dri_Pens[SHADOWPEN]      : 1;
    bg      = dri ? dri->dri_Pens[BACKGROUNDPEN]  : 0;
    text_pen = dri ? dri->dri_Pens[TEXTPEN]       : 1;

    /* Draw recessed frame */
    SetAPen(rp, shadow);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, shine);
    Move(rp, x + 1, y + h - 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + w - 1, y + 1);

    /* Fill background */
    SetAPen(rp, bg);
    RectFill(rp, x + 1, y + 1, x + w - 2, y + h - 2);

    /* Get selected item text */
    if (data->cd_Labels && data->cd_Selected >= 0)
    {
        struct ChooserNode *cn = chooser_get_node(data->cd_Labels,
                                                  data->cd_Selected);
        if (cn)
            label = cn->cn_Text;
    }
    if (!label && data->cd_Title)
        label = data->cd_Title;

    /* Draw label text */
    if (label)
    {
        WORD tx = x + 4;
        WORD ty = y + (h - rp->TxHeight) / 2 + rp->TxBaseline;

        SetAPen(rp, text_pen);
        SetDrMd(rp, JAM1);
        Move(rp, tx, ty);
        Text(rp, label, strlen(label));
    }

    /* Draw dropdown arrow on the right */
    if (data->cd_DropDown)
    {
        WORD aw = 16;

        /* Separator line */
        SetAPen(rp, shadow);
        Move(rp, x + w - aw - 1, y + 1);
        Draw(rp, x + w - aw - 1, y + h - 2);
        SetAPen(rp, shine);
        Move(rp, x + w - aw, y + 1);
        Draw(rp, x + w - aw, y + h - 2);

        /* Arrow */
        chooser_draw_arrow(rp, x + w - aw, y, aw, h, text_pen);
    }

    /* Disabled overlay */
    if (gad->Flags & GFLG_DISABLED)
    {
        ULONG pattern[] = { 0x88888888, 0x22222222 };
        SetAPen(rp, bg);
        SetAfPt(rp, (UWORD *)pattern, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}

/******************************************************************************/

IPTR Chooser__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);

    if (gad->Flags & GFLG_DISABLED)
        return GMR_NOREUSE;

    data->cd_Active = TRUE;

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR Chooser__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);
    struct InputEvent *ie = msg->gpi_IEvent;

    if (!ie)
        return GMR_MEACTIVE;

    if (ie->ie_Class == IECLASS_RAWMOUSE)
    {
        if (ie->ie_Code == SELECTUP)
        {
            WORD mx = msg->gpi_Mouse.X;
            WORD my = msg->gpi_Mouse.Y;

            /* Check if mouse is within gadget bounds */
            if (mx >= 0 && mx < gad->Width &&
                my >= 0 && my < gad->Height)
            {
                /* Cycle to next item */
                if (data->cd_NumLabels > 0)
                {
                    data->cd_Selected++;
                    if (data->cd_Selected >= data->cd_NumLabels)
                        data->cd_Selected = 0;
                }

                /* Redraw */
                struct RastPort *rp = ObtainGIRPort(msg->gpi_GInfo);
                if (rp)
                {
                    struct gpRender render;
                    render.MethodID   = GM_RENDER;
                    render.gpr_GInfo  = msg->gpi_GInfo;
                    render.gpr_RPort  = rp;
                    render.gpr_Redraw = GREDRAW_REDRAW;
                    DoMethodA(o, (Msg)&render);
                    ReleaseGIRPort(rp);
                }

                *msg->gpi_Termination = data->cd_Selected;
                return GMR_NOREUSE | GMR_VERIFY;
            }
            else
            {
                return GMR_NOREUSE;
            }
        }
        else if (ie->ie_Code == MENUDOWN)
        {
            return GMR_REUSE;
        }
    }

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR Chooser__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct ChooserData *data = INST_DATA(cl, o);

    data->cd_Active = FALSE;

    /* Redraw in inactive state */
    if (msg->gpgi_GInfo)
    {
        struct RastPort *rp = ObtainGIRPort(msg->gpgi_GInfo);
        if (rp)
        {
            struct gpRender render;
            render.MethodID   = GM_RENDER;
            render.gpr_GInfo  = msg->gpgi_GInfo;
            render.gpr_RPort  = rp;
            render.gpr_Redraw = GREDRAW_REDRAW;
            DoMethodA(o, (Msg)&render);
            ReleaseGIRPort(rp);
        }
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}
