/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction clicktab.gadget - BOOPSI class implementation
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
#include <gadgets/clicktab.h>
#include <utility/tagitem.h>

#include <string.h>

#include "clicktab_intern.h"

#define ClickTabBase ((struct Library *)(cl->cl_UserData))

#define TAB_PADDING_H   12
#define TAB_PADDING_V   4
#define TAB_OVERLAP     2

/******************************************************************************/

static LONG clicktab_count_tabs(struct List *labels)
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

static struct ClickTabNode *clicktab_get_node(struct List *labels, LONG index)
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
                return (struct ClickTabNode *)node;
        }
    }
    return NULL;
}

static void clicktab_free_widths(struct ClickTabData *data)
{
    if (data->td_TabWidths)
    {
        FreeVec(data->td_TabWidths);
        data->td_TabWidths = NULL;
    }
}

static void clicktab_compute_widths(struct ClickTabData *data,
                                    struct RastPort *rp)
{
    LONG count = data->td_NumTabs;

    clicktab_free_widths(data);

    if (count <= 0 || !data->td_Labels)
        return;

    data->td_TabWidths = (UWORD *)AllocVec(count * sizeof(UWORD),
                                            MEMF_PUBLIC | MEMF_CLEAR);
    if (data->td_TabWidths)
    {
        struct MinNode *node;
        LONG i = 0;

        for (node = (struct MinNode *)data->td_Labels->lh_Head;
             node->mln_Succ && i < count;
             node = node->mln_Succ, i++)
        {
            struct ClickTabNode *tn = (struct ClickTabNode *)node;
            UWORD w = TAB_PADDING_H * 2;

            if (tn->tn_Text && rp)
            {
                struct TextExtent te;
                TextExtent(rp, tn->tn_Text, strlen(tn->tn_Text), &te);
                w += te.te_Width;
            }
            else
            {
                w += 40;
            }

            data->td_TabWidths[i] = w;
        }

        data->td_TabHeight = rp ? rp->TxHeight + TAB_PADDING_V * 2 : 18;
    }
}

/* Determine which tab index an x coordinate falls in */
static LONG clicktab_hit_test(struct ClickTabData *data, WORD mx)
{
    LONG i;
    WORD tx = 0;

    if (!data->td_TabWidths)
        return -1;

    for (i = 0; i < data->td_NumTabs; i++)
    {
        if (mx >= tx && mx < tx + data->td_TabWidths[i])
            return i;
        tx += data->td_TabWidths[i] - TAB_OVERLAP;
    }
    return -1;
}

/******************************************************************************/

static void clicktab_set(Class *cl, Object *o, struct opSet *msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case CLICKTAB_Labels:
                data->td_Labels = (struct List *)tag->ti_Data;
                data->td_NumTabs = clicktab_count_tabs(data->td_Labels);
                clicktab_free_widths(data);
                break;
            case CLICKTAB_Current:
                data->td_Current = (LONG)tag->ti_Data;
                break;
            case CLICKTAB_PageGroup:
                data->td_PageGroup = (Object *)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR ClickTab__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct ClickTabData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct ClickTabData));
        data->td_Current = 0;
        data->td_HoverTab = -1;

        clicktab_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR ClickTab__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);

    clicktab_free_widths(data);

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR ClickTab__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    clicktab_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR ClickTab__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case CLICKTAB_Current:
            *msg->opg_Storage = data->td_Current;
            return TRUE;

        case CLICKTAB_Labels:
            *msg->opg_Storage = (IPTR)data->td_Labels;
            return TRUE;

        case CLICKTAB_PageGroup:
            *msg->opg_Storage = (IPTR)data->td_PageGroup;
            return TRUE;

        case CLICKTAB_CurrentNode:
            *msg->opg_Storage = (IPTR)clicktab_get_node(data->td_Labels,
                                                         data->td_Current);
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

static void clicktab_draw_tab(struct RastPort *rp, struct DrawInfo *dri,
                              WORD x, WORD y, WORD w, WORD h,
                              STRPTR text, BOOL selected, BOOL disabled)
{
    UWORD shine   = dri ? dri->dri_Pens[SHINEPEN]      : 2;
    UWORD shadow  = dri ? dri->dri_Pens[SHADOWPEN]     : 1;
    UWORD bg      = dri ? dri->dri_Pens[BACKGROUNDPEN] : 0;
    UWORD fill    = dri ? dri->dri_Pens[FILLPEN]       : 3;
    UWORD textpen = dri ? dri->dri_Pens[TEXTPEN]       : 1;

    /* Fill tab background */
    SetAPen(rp, selected ? bg : fill);
    RectFill(rp, x + 1, y + 1, x + w - 2, y + h - 1);

    /* Left edge */
    SetAPen(rp, shine);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y + 2);
    Draw(rp, x + 1, y + 1);
    Draw(rp, x + 2, y);

    /* Top edge */
    Draw(rp, x + w - 3, y);

    /* Right edge */
    SetAPen(rp, shadow);
    Move(rp, x + w - 2, y + 1);
    Draw(rp, x + w - 1, y + 2);
    Draw(rp, x + w - 1, y + h - 1);

    /* Bottom line for non-selected tabs */
    if (!selected)
    {
        SetAPen(rp, shadow);
        Move(rp, x, y + h - 1);
        Draw(rp, x + w - 1, y + h - 1);
    }
    else
    {
        /* Erase the bottom line for the selected tab */
        SetAPen(rp, bg);
        Move(rp, x + 1, y + h - 1);
        Draw(rp, x + w - 2, y + h - 1);
    }

    /* Draw label */
    if (text)
    {
        WORD tx = x + (w - TextLength(rp, text, strlen(text))) / 2;
        WORD ty = y + TAB_PADDING_V + rp->TxBaseline;

        if (!selected)
            ty++;

        SetAPen(rp, textpen);
        SetDrMd(rp, JAM1);
        Move(rp, tx, ty);
        Text(rp, text, strlen(text));
    }

    /* Disabled ghost pattern */
    if (disabled)
    {
        ULONG pattern[] = { 0x88888888, 0x22222222 };
        SetAPen(rp, bg);
        SetAfPt(rp, (UWORD *)pattern, 1);
        RectFill(rp, x + 1, y + 1, x + w - 2, y + h - 2);
        SetAfPt(rp, NULL, 0);
    }
}

IPTR ClickTab__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    WORD tx;
    LONG i;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    /* Compute tab widths if not yet done */
    if (!data->td_TabWidths)
        clicktab_compute_widths(data, rp);

    /* Clear gadget area */
    SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    if (!data->td_Labels || !data->td_TabWidths)
        goto done;

    /* Draw bottom line across entire width */
    {
        UWORD shadow = dri ? dri->dri_Pens[SHADOWPEN] : 1;
        SetAPen(rp, shadow);
        Move(rp, x, y + data->td_TabHeight - 1);
        Draw(rp, x + w - 1, y + data->td_TabHeight - 1);
    }

    /* Draw each tab - non-selected first, selected last (on top) */
    tx = x;
    for (i = 0; i < data->td_NumTabs; i++)
    {
        if (i != data->td_Current)
        {
            struct ClickTabNode *tn = clicktab_get_node(data->td_Labels, i);
            STRPTR label = tn ? tn->tn_Text : NULL;
            BOOL disabled = tn ? tn->tn_Disabled : FALSE;

            clicktab_draw_tab(rp, dri, tx, y,
                              data->td_TabWidths[i], data->td_TabHeight,
                              label, FALSE, disabled);
        }
        tx += data->td_TabWidths[i] - TAB_OVERLAP;
    }

    /* Draw selected tab on top */
    if (data->td_Current >= 0 && data->td_Current < data->td_NumTabs)
    {
        struct ClickTabNode *tn = clicktab_get_node(data->td_Labels,
                                                     data->td_Current);
        STRPTR label = tn ? tn->tn_Text : NULL;

        /* Recompute x position of the selected tab */
        tx = x;
        for (i = 0; i < data->td_Current; i++)
            tx += data->td_TabWidths[i] - TAB_OVERLAP;

        clicktab_draw_tab(rp, dri, tx, y,
                          data->td_TabWidths[data->td_Current],
                          data->td_TabHeight,
                          label, TRUE, FALSE);
    }

done:
    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}

/******************************************************************************/

IPTR ClickTab__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    struct Gadget *gad = G(o);

    if (gad->Flags & GFLG_DISABLED)
        return GMR_NOREUSE;

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR ClickTab__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);
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

            /* Check within tab bar area */
            if (mx >= 0 && mx < gad->Width &&
                my >= 0 && my < (WORD)data->td_TabHeight)
            {
                LONG hit = clicktab_hit_test(data, mx);

                if (hit >= 0 && hit != data->td_Current)
                {
                    struct ClickTabNode *tn =
                        clicktab_get_node(data->td_Labels, hit);

                    if (tn && !tn->tn_Disabled)
                    {
                        data->td_Current = hit;

                        /* Redraw tabs */
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

                        *msg->gpi_Termination = data->td_Current;
                        return GMR_NOREUSE | GMR_VERIFY;
                    }
                }
            }

            return GMR_NOREUSE;
        }
        else if (ie->ie_Code == MENUDOWN)
        {
            return GMR_REUSE;
        }
    }

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR ClickTab__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);

    data->td_HoverTab = -1;

    return DoSuperMethodA(cl, o, (Msg)msg);
}
