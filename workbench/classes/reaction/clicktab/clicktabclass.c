/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction clicktab.gadget - BOOPSI class implementation
*/
#define DEBUG 1

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
#include <gadgets/layout.h>
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

/* Free a labels list previously built by clicktab_build_auto_labels(). */
static void clicktab_free_auto_labels(struct ClickTabData *data)
{
    if (data->td_AutoLabels)
    {
        struct ClickTabNode *tn;
        while ((tn = (struct ClickTabNode *)RemHead(data->td_AutoLabels)))
            FreeVec(tn);
        FreeVec(data->td_AutoLabels);
        data->td_AutoLabels = NULL;
    }
}

/* Build an internal ClickTabNode list from a NULL-terminated STRPTR[]
   passed via GA_Text. The labels themselves are kept by the caller. */
static struct List *clicktab_build_auto_labels(STRPTR *array)
{
    struct List *list;
    LONG i;

    if (!array)
        return NULL;

    list = (struct List *)AllocVec(sizeof(struct List),
                                    MEMF_PUBLIC | MEMF_CLEAR);
    if (!list)
        return NULL;

    NewList(list);

    for (i = 0; array[i]; i++)
    {
        struct ClickTabNode *tn;

        tn = (struct ClickTabNode *)AllocVec(sizeof(struct ClickTabNode),
                                              MEMF_PUBLIC | MEMF_CLEAR);
        if (!tn)
            break;

        tn->tn_Text   = array[i];
        tn->tn_Number = i;
        AddTail(list, (struct Node *)tn);
    }

    return list;
}

static UWORD clicktab_label_width(struct RastPort *rp, STRPTR text);
static void  clicktab_draw_label(struct RastPort *rp, WORD x, WORD y,
                                  STRPTR text);

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
                w += clicktab_label_width(rp, tn->tn_Text);
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

/* Locate the first underscore in text (the AmigaOS convention for
   marking the keyboard shortcut letter). Returns the byte offset
   of the underscore, or -1 if none found. The shortcut character
   itself is at offset+1. */
static LONG clicktab_find_underscore(STRPTR text)
{
    if (text)
    {
        LONG i;
        for (i = 0; text[i]; i++)
        {
            if (text[i] == '_' && text[i + 1])
                return i;
        }
    }
    return -1;
}

/* Pixel width of a label, ignoring the underscore marker. */
static UWORD clicktab_label_width(struct RastPort *rp, STRPTR text)
{
    LONG us;
    UWORD w = 0;

    if (!text || !rp)
        return 0;

    us = clicktab_find_underscore(text);
    if (us < 0)
    {
        struct TextExtent te;
        TextExtent(rp, text, strlen(text), &te);
        w = te.te_Width;
    }
    else
    {
        struct TextExtent te;
        if (us > 0)
        {
            TextExtent(rp, text, us, &te);
            w += te.te_Width;
        }
        TextExtent(rp, text + us + 1, strlen(text + us + 1), &te);
        w += te.te_Width;
    }
    return w;
}

/* Draw a label honouring the underscore-shortcut convention: the
   underscore is suppressed and the following character is underlined. */
static void clicktab_draw_label(struct RastPort *rp, WORD x, WORD y,
                                 STRPTR text)
{
    LONG us;
    LONG len;

    if (!text)
        return;

    us  = clicktab_find_underscore(text);
    len = (LONG)strlen(text);

    if (us < 0)
    {
        Move(rp, x, y);
        Text(rp, text, len);
        return;
    }

    Move(rp, x, y);
    if (us > 0)
        Text(rp, text, us);

    /* Underline the shortcut character */
    {
        struct TextExtent te;
        WORD chx = rp->cp_x;
        UWORD chw;

        TextExtent(rp, text + us + 1, 1, &te);
        chw = te.te_Width;

        Text(rp, text + us + 1, 1);
        Move(rp, chx, y + 1);
        Draw(rp, chx + chw - 1, y + 1);
        Move(rp, chx + chw, y);
    }

    if (us + 2 < len)
        Text(rp, text + us + 2, len - (us + 2));
}

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
    BOOL currentChanged = FALSE;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case CLICKTAB_Labels:
                data->td_Labels = (struct List *)tag->ti_Data;
                data->td_NumTabs = clicktab_count_tabs(data->td_Labels);
                clicktab_free_widths(data);
                /* Caller-owned list overrides any GA_Text auto list. */
                clicktab_free_auto_labels(data);
                break;
            case GA_Text:
                /* On AmigaOS ClickTab, GA_Text accepts a NULL-terminated
                   STRPTR[] as a convenience for setting tab labels.
                   Build an internal node list and use it as td_Labels. */
                clicktab_free_auto_labels(data);
                data->td_AutoLabels =
                    clicktab_build_auto_labels((STRPTR *)tag->ti_Data);
                if (data->td_AutoLabels)
                {
                    data->td_Labels = data->td_AutoLabels;
                    data->td_NumTabs =
                        clicktab_count_tabs(data->td_Labels);
                    clicktab_free_widths(data);
                }
                break;
            case CLICKTAB_Current:
                if (data->td_Current != (LONG)tag->ti_Data)
                {
                    data->td_Current = (LONG)tag->ti_Data;
                    currentChanged = TRUE;
                }
                break;
            case CLICKTAB_PageGroup:
                data->td_PageGroup = (Object *)tag->ti_Data;
                break;
        }
    }

    if (currentChanged && data->td_PageGroup)
    {
        SetAttrs(data->td_PageGroup,
                 PAGE_Current, (IPTR)data->td_Current,
                 TAG_END);
    }
}

/******************************************************************************/

IPTR ClickTab__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[ClickTab] OM_NEW: enter\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    D(bug("[ClickTab] OM_NEW: obj=%p\n", (void *)retval));
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
    D(bug("[ClickTab] OM_DISPOSE: obj=%p\n", (void *)o));

    struct ClickTabData *data = INST_DATA(cl, o);

    clicktab_free_widths(data);
    clicktab_free_auto_labels(data);

    if (data->td_PageGroup)
    {
        DisposeObject(data->td_PageGroup);
        data->td_PageGroup = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR ClickTab__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[ClickTab] OM_SET: obj=%p\n", (void *)o));

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
        UWORD lblW = clicktab_label_width(rp, text);
        WORD tx = x + (w - lblW) / 2;
        WORD ty = y + TAB_PADDING_V + rp->TxBaseline;

        if (!selected)
            ty++;

        SetAPen(rp, textpen);
        SetDrMd(rp, JAM1);
        clicktab_draw_label(rp, tx, ty, text);
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
    D(bug("[ClickTab] GM_RENDER: obj=%p redraw=%ld\n", (void *)o, msg->gpr_Redraw));

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

    /* Clear only the tab strip area; the page area below is owned by
     * the page group and would be cleared/rendered by it. Filling the
     * full ClickTab box here would overpaint the new page's gadgets
     * after a tab switch. */
    SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
    {
        WORD th = data->td_TabHeight ? data->td_TabHeight : h;
        if (th > h) th = h;
        RectFill(rp, x, y, x + w - 1, y + th - 1);
    }

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
    /* Forward render to the page group (the panel below the tab strip) */
    if (data->td_PageGroup)
    {
        struct gpRender prender;
        memset(&prender, 0, sizeof(prender));
        prender.MethodID  = GM_RENDER;
        prender.gpr_GInfo = msg->gpr_GInfo;
        prender.gpr_RPort = rp;
        prender.gpr_Redraw = msg->gpr_Redraw;
        DoMethodA(data->td_PageGroup, (Msg)&prender);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}

/******************************************************************************/

IPTR ClickTab__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    D(bug("[ClickTab] GM_GOACTIVE: obj=%p\n", (void *)o));

    struct Gadget *gad = G(o);

    if (gad->Flags & GFLG_DISABLED)
        return GMR_NOREUSE;

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR ClickTab__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    D(bug("[ClickTab] GM_HANDLEINPUT: obj=%p\n", (void *)o));

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

                        /* Sync page group to the new current tab via
                         * SetGadgetAttrs so Page__OM_SET sees a live
                         * GadgetInfo and performs the LM_REMOVEFROMWINDOW/
                         * LM_ADDTOWINDOW dance for the old/new pages. */
                        if (data->td_PageGroup)
                        {
                            struct GadgetInfo *gi = msg->gpi_GInfo;
                            SetGadgetAttrs((struct Gadget *)data->td_PageGroup,
                                           gi ? gi->gi_Window    : NULL,
                                           gi ? gi->gi_Requester : NULL,
                                           PAGE_Current, (IPTR)data->td_Current,
                                           TAG_END);
                        }

                        /* Redraw tabs + page group */
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

/******************************************************************************/

/* GM_DOMAIN: tab strip min size + page group min size (max of all pages). */
IPTR ClickTab__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpd_RPort;
    UWORD  tabsW = 0;
    UWORD  tabsH = 18;
    UWORD  pageW = 0;
    UWORD  pageH = 0;

    /* Tab strip */
    if (rp)
    {
        clicktab_compute_widths(data, rp);
        tabsH = data->td_TabHeight ? data->td_TabHeight : 18;
        if (data->td_TabWidths)
        {
            LONG i;
            for (i = 0; i < data->td_NumTabs; i++)
                tabsW += data->td_TabWidths[i] - TAB_OVERLAP;
            if (data->td_NumTabs > 0)
                tabsW += TAB_OVERLAP;
        }
    }
    if (tabsW == 0) tabsW = 40;

    /* Page group */
    if (data->td_PageGroup)
    {
        struct gpDomain pgd;
        memset(&pgd, 0, sizeof(pgd));
        pgd.MethodID = GM_DOMAIN;
        pgd.gpd_GInfo = msg->gpd_GInfo;
        pgd.gpd_RPort = rp;
        pgd.gpd_Which = msg->gpd_Which;
        if (DoMethodA(data->td_PageGroup, (Msg)&pgd))
        {
            pageW = (UWORD)pgd.gpd_Domain.Width;
            pageH = (UWORD)pgd.gpd_Domain.Height;
        }
    }

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = (tabsW > pageW) ? tabsW : pageW;
    msg->gpd_Domain.Height = tabsH + pageH;

    D(bug("[ClickTab] GM_DOMAIN: obj=%p tabs %dx%d page %dx%d -> %dx%d\n",
        (void *)o, (int)tabsW, (int)tabsH, (int)pageW, (int)pageH,
        (int)msg->gpd_Domain.Width, (int)msg->gpd_Domain.Height));

    return TRUE;
}

/******************************************************************************/

/* GM_LAYOUT: position the page group below the tab strip and forward. */
IPTR ClickTab__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);

    D(bug("[ClickTab] GM_LAYOUT: obj=%p (%d,%d %dx%d)\n", (void *)o,
        (int)gad->LeftEdge, (int)gad->TopEdge,
        (int)gad->Width, (int)gad->Height));

    if (data->td_PageGroup)
    {
        struct Gadget *pg = (struct Gadget *)data->td_PageGroup;
        UWORD tabsH = data->td_TabHeight ? data->td_TabHeight : 18;

        pg->LeftEdge = gad->LeftEdge;
        pg->TopEdge  = gad->TopEdge + tabsH;
        pg->Width    = gad->Width;
        pg->Height   = (gad->Height > tabsH) ? (gad->Height - tabsH) : 0;

        struct gpLayout gpl;
        memset(&gpl, 0, sizeof(gpl));
        gpl.MethodID    = GM_LAYOUT;
        gpl.gpl_GInfo   = msg->gpl_GInfo;
        gpl.gpl_Initial = msg->gpl_Initial;
        DoMethodA(data->td_PageGroup, (Msg)&gpl);
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

/* LM_ADDTOWINDOW: forward to page group so its gadgets join the GList. */
IPTR ClickTab__LM_ADDTOWINDOW(Class *cl, Object *o, Msg msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);
    IPTR retval = DoSuperMethodA(cl, o, msg);

    if (data->td_PageGroup)
    {
        D(bug("[ClickTab] LM_ADDTOWINDOW: forwarding to page group %p\n",
            data->td_PageGroup));
        DoMethodA(data->td_PageGroup, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR ClickTab__LM_REMOVEFROMWINDOW(Class *cl, Object *o, Msg msg)
{
    struct ClickTabData *data = INST_DATA(cl, o);

    if (data->td_PageGroup)
    {
        D(bug("[ClickTab] LM_REMOVEFROMWINDOW: forwarding to page group %p\n",
            data->td_PageGroup));
        DoMethodA(data->td_PageGroup, msg);
    }

    return DoSuperMethodA(cl, o, msg);
}
