/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: page.gadget - BOOPSI class implementation

    page.gadget is a self-contained gadgetclass subclass that holds a
    list of children (added via PAGE_Add) and renders / hit-tests only
    the currently-selected one (PAGE_Current).

    It composes with layout.gadget as a peer rather than a subclass:
      - GM_DOMAIN: returns the max() of children's natural sizes.
      - GM_LAYOUT: assigns the full content area to every child and
        forwards GM_LAYOUT to the current page's child.
      - GM_RENDER: draws only the current page (forwarding GM_RENDER
        to it if it implements one).
      - GM_HITTEST: returns 0 (children have already been linked into
        the window's GList by LM_ADDTOWINDOW).
      - LM_ADDTOWINDOW / LM_REMOVEFROMWINDOW: forward to the current
        child only — switching pages will tear the old page down and
        bring up the new one.
      - PAGE_Current change: removes the old current's gadgets and
        adds the new current's, then refreshes.
*/
#define DEBUG 1

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
#include <gadgets/layout.h>
#include <utility/tagitem.h>

#include <string.h>

#include "page_intern.h"

#define PageBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static struct PageChild *page_alloc_child(Object *child)
{
    struct PageChild *pc = AllocVec(sizeof(struct PageChild), MEMF_CLEAR);
    if (pc)
        pc->pc_Object = child;
    return pc;
}

static void page_free_child(struct PageChild *pc)
{
    if (!pc) return;
    if (pc->pc_Object && !pc->pc_NoDispose)
        DisposeObject(pc->pc_Object);
    FreeVec(pc);
}

/* Return the n-th child or NULL. */
static struct PageChild *page_nth_child(struct PageData *data, ULONG n)
{
    struct PageChild *pc;
    ULONG i = 0;
    ForeachNode(&data->pd_Children, pc)
    {
        if (i++ == n) return pc;
    }
    return NULL;
}

/******************************************************************************/

static void page_set(Class *cl, Object *o, struct opSet *msg,
                     struct PageChild **outAdded, BOOL *outChanged)
{
    struct PageData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
        case PAGE_Add:
        {
            struct PageChild *pc = page_alloc_child((Object *)tag->ti_Data);
            if (pc)
            {
                AddTail((struct List *)&data->pd_Children, (struct Node *)pc);
                data->pd_Count++;
                if (outAdded) *outAdded = pc;
            }
            break;
        }

        case PAGE_Remove:
        {
            Object *child = (Object *)tag->ti_Data;
            struct PageChild *pc, *next;
            ForeachNodeSafe(&data->pd_Children, pc, next)
            {
                if (pc->pc_Object == child)
                {
                    Remove((struct Node *)pc);
                    if (data->pd_Count) data->pd_Count--;
                    page_free_child(pc);
                    break;
                }
            }
            break;
        }

        case PAGE_Current:
            if (data->pd_Current != (ULONG)tag->ti_Data)
            {
                data->pd_Current = (ULONG)tag->ti_Data;
                if (outChanged) *outChanged = TRUE;
            }
            break;

        case PAGE_FixedHoriz:
            data->pd_FixedHoriz = (BOOL)tag->ti_Data;
            break;

        case PAGE_FixedVert:
            data->pd_FixedVert = (BOOL)tag->ti_Data;
            break;

        case PAGE_Transparent:
            data->pd_Transparent = (BOOL)tag->ti_Data;
            break;
        }
    }
}

/******************************************************************************/

IPTR Page__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[Page] OM_NEW: entry\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct PageData *data = INST_DATA(cl, (Object *)retval);
        memset(data, 0, sizeof(*data));
        NewList((struct List *)&data->pd_Children);
        data->pd_Current = 0;

        page_set(cl, (Object *)retval, msg, NULL, NULL);

        D(bug("[Page] OM_NEW: created obj 0x%p children=%ld current=%ld\n",
            (APTR)retval, (LONG)data->pd_Count, (LONG)data->pd_Current));
    }
    return retval;
}

/******************************************************************************/

IPTR Page__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct PageData *data = INST_DATA(cl, o);
    struct PageChild *pc, *next;

    D(bug("[Page] OM_DISPOSE: obj 0x%p\n", o));

    ForeachNodeSafe(&data->pd_Children, pc, next)
    {
        Remove((struct Node *)pc);
        page_free_child(pc);
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Page__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct PageData *data = INST_DATA(cl, o);
    BOOL changed = FALSE;
    ULONG oldCurrent = data->pd_Current;
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    page_set(cl, o, msg, NULL, &changed);

    /* If the current page was changed in a live window context (signalled
     * via OPUF_INTERIM == 0 and a non-NULL ops_GInfo), tear down the old
     * page's gadgets and bring up the new one. We rely on Intuition's
     * normal SetGadgetAttrs / RethinkLayout flow to issue the refresh. */
    if (changed && msg->ops_GInfo && msg->ops_GInfo->gi_Window)
    {
        struct Window *win = msg->ops_GInfo->gi_Window;
        struct Requester *req = msg->ops_GInfo->gi_Requester;
        struct PageChild *oldPc = page_nth_child(data, oldCurrent);
        struct PageChild *newPc = page_nth_child(data, data->pd_Current);
        struct lpAddToWindow lpa;

        D(bug("[Page] OM_SET: PAGE_Current %ld -> %ld\n",
            (LONG)oldCurrent, (LONG)data->pd_Current));

        if (oldPc && oldPc->pc_Object)
        {
            lpa.MethodID       = LM_REMOVEFROMWINDOW;
            lpa.lpaw_Window    = win;
            lpa.lpaw_Requester = req;
            DoMethodA(oldPc->pc_Object, (Msg)&lpa);
        }

        if (newPc && newPc->pc_Object)
        {
            /* Trigger geometry recalc on the new page so it gets the
             * current GadgetBox before being rendered. */
            struct gpLayout gpl;
            memset(&gpl, 0, sizeof(gpl));
            gpl.MethodID = GM_LAYOUT;
            gpl.gpl_GInfo = msg->ops_GInfo;
            gpl.gpl_Initial = 1;
            DoMethodA(newPc->pc_Object, (Msg)&gpl);

            /* Erase the page area first, so the LM_ADDTOWINDOW that
             * follows (which internally RefreshGList's the new page's
             * gadgets) paints onto a clean background instead of being
             * overpainted by our fill. */
            {
                struct RastPort *rp = ObtainGIRPort(msg->ops_GInfo);
                if (rp)
                {
                    SetAPen(rp, msg->ops_GInfo->gi_DrInfo->dri_Pens[BACKGROUNDPEN]);
                    RectFill(rp, G(o)->LeftEdge, G(o)->TopEdge,
                        G(o)->LeftEdge + G(o)->Width  - 1,
                        G(o)->TopEdge  + G(o)->Height - 1);
                    ReleaseGIRPort(rp);
                }
            }

            lpa.MethodID       = LM_ADDTOWINDOW;
            lpa.lpaw_Window    = win;
            lpa.lpaw_Requester = req;
            DoMethodA(newPc->pc_Object, (Msg)&lpa);
        }
    }

    return retval;
}

/******************************************************************************/

IPTR Page__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct PageData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
    case PAGE_Current:
        *msg->opg_Storage = data->pd_Current;
        return TRUE;
    }
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Page__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct PageData *data = INST_DATA(cl, o);
    struct PageChild *pc;
    ULONG maxW = 0, maxH = 0;

    ForeachNode(&data->pd_Children, pc)
    {
        struct gpDomain gpd;
        if (!pc->pc_Object) continue;
        memset(&gpd, 0, sizeof(gpd));
        gpd.MethodID  = GM_DOMAIN;
        gpd.gpd_GInfo = msg->gpd_GInfo;
        gpd.gpd_RPort = msg->gpd_RPort;
        gpd.gpd_Which = msg->gpd_Which;
        if (DoMethodA(pc->pc_Object, (Msg)&gpd))
        {
            if ((ULONG)gpd.gpd_Domain.Width  > maxW) maxW = gpd.gpd_Domain.Width;
            if ((ULONG)gpd.gpd_Domain.Height > maxH) maxH = gpd.gpd_Domain.Height;
        }
    }
    if (maxW == 0) maxW = 40;
    if (maxH == 0) maxH = 14;
    if (maxW > 0xFFFF) maxW = 0xFFFF;
    if (maxH > 0xFFFF) maxH = 0xFFFF;

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = maxW;
    msg->gpd_Domain.Height = maxH;

    D(bug("[Page] GM_DOMAIN: obj %p -> %ldx%ld\n", o, (LONG)maxW, (LONG)maxH));
    return TRUE;
}

/******************************************************************************/

IPTR Page__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    struct PageData *data = INST_DATA(cl, o);
    struct PageChild *pc;
    WORD x = G(o)->LeftEdge;
    WORD y = G(o)->TopEdge;
    UWORD w = G(o)->Width;
    UWORD h = G(o)->Height;

    D(bug("[Page] GM_LAYOUT: obj %p box (%d,%d) %dx%d\n", o, x, y, w, h));

    /* Assign the full area to every child so a PAGE_Current switch
     * doesn't require a re-layout. */
    ForeachNode(&data->pd_Children, pc)
    {
        struct gpLayout gpl;
        if (!pc->pc_Object) continue;
        SetAttrs(pc->pc_Object,
            GA_Left,   x, GA_Top,    y,
            GA_Width,  w, GA_Height, h,
            TAG_DONE);
        memset(&gpl, 0, sizeof(gpl));
        gpl.MethodID  = GM_LAYOUT;
        gpl.gpl_GInfo = msg->gpl_GInfo;
        gpl.gpl_Initial = msg->gpl_Initial;
        DoMethodA(pc->pc_Object, (Msg)&gpl);
    }
    return TRUE;
}

/******************************************************************************/

IPTR Page__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct PageData *data = INST_DATA(cl, o);
    struct PageChild *pc;

    D(bug("[Page] GM_RENDER: obj %p current=%ld\n", o, (LONG)data->pd_Current));

    pc = page_nth_child(data, data->pd_Current);
    if (pc && pc->pc_Object)
        DoMethodA(pc->pc_Object, (Msg)msg);

    return TRUE;
}

/******************************************************************************/

IPTR Page__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
    /* Children are linked into the window's GList by LM_ADDTOWINDOW;
     * Intuition hit-tests them directly. */
    return 0;
}

/******************************************************************************/

IPTR Page__LM_ADDTOWINDOW(Class *cl, Object *o, struct lpAddToWindow *msg)
{
    struct PageData *data = INST_DATA(cl, o);
    struct PageChild *pc = page_nth_child(data, data->pd_Current);

    D(bug("[Page] LM_ADDTOWINDOW: obj %p current=%ld child=%p\n",
        o, (LONG)data->pd_Current, pc ? pc->pc_Object : NULL));

    if (pc && pc->pc_Object)
        return DoMethodA(pc->pc_Object, (Msg)msg);

    return 0;
}

/******************************************************************************/

IPTR Page__LM_REMOVEFROMWINDOW(Class *cl, Object *o, struct lpAddToWindow *msg)
{
    struct PageData *data = INST_DATA(cl, o);
    struct PageChild *pc = page_nth_child(data, data->pd_Current);

    D(bug("[Page] LM_REMOVEFROMWINDOW: obj %p\n", o));

    if (pc && pc->pc_Object)
        return DoMethodA(pc->pc_Object, (Msg)msg);

    return 0;
}
