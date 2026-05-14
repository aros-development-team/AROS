/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction layout.gadget - BOOPSI class implementation
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
#include <images/bevel.h>
#include <utility/tagitem.h>
#include <reaction/reaction_prefs.h>

#include <string.h>

#include "layout_intern.h"

#define LayoutBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static struct LayoutChild *alloc_child(void)
{
    struct LayoutChild *lc = AllocVec(sizeof(struct LayoutChild), MEMF_CLEAR);
    if (lc)
    {
        lc->lc_WeightedWidth  = 100;
        lc->lc_WeightedHeight = 100;
        lc->lc_MaxWidth  = 0xFFFF;
        lc->lc_MaxHeight = 0xFFFF;
    }
    return lc;
}

static void free_child(struct LayoutChild *lc)
{
    if (lc)
    {
        if (!lc->lc_NoDispose && lc->lc_Object)
            DisposeObject(lc->lc_Object);
        if (lc->lc_Label)
            DisposeObject(lc->lc_Label);
        FreeVec(lc);
    }
}

/******************************************************************************/

static void process_child_tags(struct LayoutChild *lc, struct TagItem *tags)
{
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case CHILD_MinWidth:
                lc->lc_MinWidth = (UWORD)tag->ti_Data;
                break;
            case CHILD_MinHeight:
                lc->lc_MinHeight = (UWORD)tag->ti_Data;
                break;
            case CHILD_MaxWidth:
                lc->lc_MaxWidth = (UWORD)tag->ti_Data;
                break;
            case CHILD_MaxHeight:
                lc->lc_MaxHeight = (UWORD)tag->ti_Data;
                break;
            case CHILD_WeightedWidth:
                lc->lc_WeightedWidth = (UWORD)tag->ti_Data;
                break;
            case CHILD_WeightedHeight:
                lc->lc_WeightedHeight = (UWORD)tag->ti_Data;
                break;
            case CHILD_NominalSize:
                lc->lc_NominalSize = (BOOL)tag->ti_Data;
                break;
            case CHILD_ScaleWidth:
                lc->lc_ScaleWidth = (BOOL)tag->ti_Data;
                break;
            case CHILD_ScaleHeight:
                lc->lc_ScaleHeight = (BOOL)tag->ti_Data;
                break;
            case CHILD_NoDispose:
                lc->lc_NoDispose = (BOOL)tag->ti_Data;
                break;
            case CHILD_CacheDomain:
                lc->lc_CacheDomain = (BOOL)tag->ti_Data;
                break;
            case CHILD_WeightMinimum:
                lc->lc_WeightMinimum = (BOOL)tag->ti_Data;
                break;
            case CHILD_Label:
                lc->lc_Label = (Object *)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

static void layout_set(Class *cl, Object *o, struct opSet *msg)
{
    struct LayoutData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;
    struct LayoutChild *lc;

    D(bug("[Layout] layout_set: obj 0x%p\n", o));

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LAYOUT_Orientation:
                data->ld_Orientation = tag->ti_Data;
                break;

            case LAYOUT_SpaceOuter:
                data->ld_SpaceOuter = (BOOL)tag->ti_Data;
                break;

            case LAYOUT_SpaceInner:
                data->ld_SpaceInner = (BOOL)tag->ti_Data;
                break;

            case LAYOUT_BevelStyle:
                if (data->ld_BevelStyle != tag->ti_Data && data->ld_BevelImage)
                {
                    DisposeObject(data->ld_BevelImage);
                    data->ld_BevelImage = NULL;
                }
                data->ld_BevelStyle = tag->ti_Data;
                data->ld_DomainValid = FALSE;
                break;

            case LAYOUT_EvenSize:
                data->ld_EvenSize = (BOOL)tag->ti_Data;
                break;

            case LAYOUT_DeferLayout:
                data->ld_DeferLayout = (BOOL)tag->ti_Data;
                break;

            case LAYOUT_HorizAlignment:
                data->ld_HorizAlignment = tag->ti_Data;
                break;

            case LAYOUT_VertAlignment:
                data->ld_VertAlignment = tag->ti_Data;
                break;

            case LAYOUT_Label:
                if (data->ld_BevelImage)
                {
                    DisposeObject(data->ld_BevelImage);
                    data->ld_BevelImage = NULL;
                }
                data->ld_Label = (STRPTR)tag->ti_Data;
                data->ld_DomainValid = FALSE;
                break;

            case LAYOUT_LabelPlace:
                data->ld_LabelPlace = tag->ti_Data;
                data->ld_LabelPlaceSet = TRUE;
                break;

            case LAYOUT_InnerSpacing:
                data->ld_HorizSpacing = (UWORD)tag->ti_Data;
                data->ld_VertSpacing = (UWORD)tag->ti_Data;
                break;

            case LAYOUT_TopSpacing:
                data->ld_TopSpacing = (UWORD)tag->ti_Data;
                break;

            case LAYOUT_BottomSpacing:
                data->ld_BottomSpacing = (UWORD)tag->ti_Data;
                break;

            case LAYOUT_LeftSpacing:
                data->ld_LeftSpacing = (UWORD)tag->ti_Data;
                break;

            case LAYOUT_RightSpacing:
                data->ld_RightSpacing = (UWORD)tag->ti_Data;
                break;

            case LAYOUT_ShrinkWrap:
                data->ld_ShrinkWrap = (BOOL)tag->ti_Data;
                break;

            case LAYOUT_AddChild:
                D(bug("[Layout] layout_set: LAYOUT_AddChild child=0x%p\n", (APTR)tag->ti_Data));
                lc = alloc_child();
                if (lc)
                {
                    lc->lc_Object = (Object *)tag->ti_Data;
                    lc->lc_IsImage = FALSE;
                    /* Process CHILD_* tags that follow */
                    process_child_tags(lc, tag + 1);
                    AddTail((struct List *)&data->ld_Children, (struct Node *)lc);
                    data->ld_DomainValid = FALSE;
                }
                break;

            case LAYOUT_AddImage:
                D(bug("[Layout] layout_set: LAYOUT_AddImage image=0x%p\n", (APTR)tag->ti_Data));
                lc = alloc_child();
                if (lc)
                {
                    lc->lc_Object = (Object *)tag->ti_Data;
                    lc->lc_IsImage = TRUE;
                    process_child_tags(lc, tag + 1);
                    AddTail((struct List *)&data->ld_Children, (struct Node *)lc);
                    data->ld_DomainValid = FALSE;
                }
                break;

            case LAYOUT_RemoveChild:
            {
                Object *child = (Object *)tag->ti_Data;
                struct LayoutChild *node, *next;

                ForeachNodeSafe(&data->ld_Children, node, next)
                {
                    if (node->lc_Object == child)
                    {
                        Remove((struct Node *)node);
                        node->lc_NoDispose = TRUE;
                        free_child(node);
                        data->ld_DomainValid = FALSE;
                        break;
                    }
                }
                break;
            }


        }
    }
}

/******************************************************************************/

IPTR Layout__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[Layout] OM_NEW: entry\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct LayoutData *data = INST_DATA(cl, (Object *)retval);
        struct UIPrefs *prefs;
        UWORD spacing = 4;
        UWORD vspacing = 2;

        memset(data, 0, sizeof(struct LayoutData));
        NewList((struct List *)&data->ld_Children);

        /* Consult live UIPrefs (published by reaction.library). The
         * semaphore is found by name; cap_Semaphore is its first member,
         * so the semaphore pointer IS the UIPrefs pointer. */
        Forbid();
        prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
        if (prefs)
            ObtainSemaphoreShared(&prefs->cap_Semaphore);
        Permit();
        if (prefs)
        {
            if (prefs->cap_LayoutSpacing)
            {
                spacing  = prefs->cap_LayoutSpacing;
                vspacing = prefs->cap_LayoutSpacing >> 1;
                if (vspacing == 0) vspacing = 1;
            }
            data->ld_3DLook = prefs->cap_3DLook ? TRUE : FALSE;
            /* Default label placement from prefs (overridden if user
             * supplies LAYOUT_LabelPlace via tags). */
            data->ld_LabelPlace = prefs->cap_LabelPlace;
            ReleaseSemaphore(&prefs->cap_Semaphore);
        }
        else
        {
            data->ld_3DLook = TRUE;
            data->ld_LabelPlace = PLACETEXT_LEFT;
        }

        /* Defaults */
        data->ld_Orientation = LAYOUT_ORIENT_HORIZ;
        data->ld_SpaceInner  = TRUE;
        data->ld_HorizSpacing = spacing;
        data->ld_VertSpacing  = vspacing;
        data->ld_TopSpacing   = spacing;
        data->ld_BottomSpacing = spacing;
        data->ld_LeftSpacing  = spacing;
        data->ld_RightSpacing = spacing;

        layout_set(cl, (Object *)retval, msg);

        D(bug("[Layout] OM_NEW: created obj 0x%p orientation=%ld\n", (APTR)retval, (LONG)data->ld_Orientation));
    }
    else
    {
        D(bug("[Layout] OM_NEW: superclass failed\n"));
    }

    return retval;
}

/******************************************************************************/

IPTR Layout__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct LayoutData *data = INST_DATA(cl, o);
    struct LayoutChild *lc, *next;

    D(bug("[Layout] OM_DISPOSE: obj 0x%p\n", o));

    ForeachNodeSafe(&data->ld_Children, lc, next)
    {
        Remove((struct Node *)lc);
        free_child(lc);
    }

    if (data->ld_BevelImage)
    {
        DisposeObject(data->ld_BevelImage);
        data->ld_BevelImage = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Layout__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;
    D(bug("[Layout] OM_SET: obj 0x%p\n", o));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    layout_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Layout__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct LayoutData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case LAYOUT_Orientation:
            *msg->opg_Storage = data->ld_Orientation;
            return TRUE;

        case LAYOUT_SpaceOuter:
            *msg->opg_Storage = data->ld_SpaceOuter;
            return TRUE;

        case LAYOUT_SpaceInner:
            *msg->opg_Storage = data->ld_SpaceInner;
            return TRUE;

        case LAYOUT_EvenSize:
            *msg->opg_Storage = data->ld_EvenSize;
            return TRUE;

        case LAYOUT_BevelStyle:
            *msg->opg_Storage = data->ld_BevelStyle;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

/*
 * Query the preferred minimum size of a child object.
 *
 * For gadget children sends GM_DOMAIN to the child (recursively for nested
 * layouts). For image children reads IA_Width/IA_Height attributes, falling
 * back to the raw Image struct fields. Honors any explicit CHILD_MinWidth/
 * CHILD_MinHeight and CHILD_NominalSize hints.
 */
static void query_child_size(struct LayoutChild *lc, struct GadgetInfo *gi,
                             struct RastPort *rp, UWORD *outW, UWORD *outH)
{
    UWORD w = lc->lc_MinWidth;
    UWORD h = lc->lc_MinHeight;

    if (lc->lc_Object)
    {
        if (lc->lc_IsImage)
        {
            IPTR iw = 0, ih = 0;
            GetAttr(IA_Width, lc->lc_Object, &iw);
            GetAttr(IA_Height, lc->lc_Object, &ih);
            /* Do NOT fall back to the underlying Image's Width/Height
             * fields if GetAttr returned 0 — imageclass leaves those at
             * a default 80x40 unless explicitly initialised, which would
             * otherwise reserve unwanted space for empty/spacer image
             * objects (e.g. an empty label.image). A class that genuinely
             * wants to advertise a non-zero natural size must override
             * IA_Width / IA_Height in OM_GET. */
            if ((UWORD)iw > w) w = (UWORD)iw;
            if ((UWORD)ih > h) h = (UWORD)ih;
        }
        else
        {
            struct gpDomain gpd;
            memset(&gpd, 0, sizeof(gpd));
            gpd.MethodID = GM_DOMAIN;
            gpd.gpd_GInfo = gi;
            gpd.gpd_RPort = rp;
            gpd.gpd_Which = lc->lc_NominalSize ? GDOMAIN_NOMINAL : GDOMAIN_MINIMUM;
            if (DoMethodA(lc->lc_Object, (Msg)&gpd))
            {
                if ((UWORD)gpd.gpd_Domain.Width  > w) w = (UWORD)gpd.gpd_Domain.Width;
                if ((UWORD)gpd.gpd_Domain.Height > h) h = (UWORD)gpd.gpd_Domain.Height;
            }
            /* Final fallback for gadgets that don't implement GM_DOMAIN */
            if (w == 0) w = 40;
            if (h == 0) h = 14;
        }
    }

    *outW = w;
    *outH = h;

    /* Cache the queried natural size so perform_layout can use it as the
     * lower bound when distributing space (independent of any explicit
     * CHILD_MinWidth/CHILD_MinHeight tag values). */
    lc->lc_NatWidth  = w;
    lc->lc_NatHeight = h;

    D(bug("[Layout] query_child: child=%p isImage=%d -> %dx%d\n",
        lc->lc_Object, (int)lc->lc_IsImage, (int)w, (int)h));
}

/* Return per-edge frame thickness for the layout's bevel style. */
static UWORD layout_frame_thickness(ULONG style)
{
    switch (style)
    {
        case BVS_NONE:    return 0;
        case BVS_THIN:
        case BVS_BOX:
        case BVS_FOCUS:   return 1;
        default:          return 2;
    }
}

/******************************************************************************/

void layout_compute_domain(Class *cl, Object *o, struct LayoutData *data)
{
    struct LayoutChild *lc;
    ULONG totalMin = 0;
    ULONG maxCross = 0;
    ULONG maxMain = 0;
    ULONG count = 0;
    UWORD spacing;
    struct GadgetInfo gi_storage;
    struct GadgetInfo *gi = NULL;
    struct RastPort *rp = NULL;
    struct Screen *scr;
    struct DrawInfo *dri = NULL;

    if (data->ld_DomainValid)
        return;

    /* Cycle guard - prevent infinite recursion through malformed object graphs */
    if (data->ld_ComputingDomain)
    {
        data->ld_MinWidth  = 0;
        data->ld_MinHeight = 0;
        return;
    }
    data->ld_ComputingDomain = TRUE;

    /* Find a usable rastport / DrawInfo for child measurement */
    scr = LockPubScreen(NULL);
    if (scr)
    {
        rp = &scr->RastPort;
        dri = GetScreenDrawInfo(scr);
        memset(&gi_storage, 0, sizeof(gi_storage));
        gi_storage.gi_Screen  = scr;
        gi_storage.gi_RastPort = rp;
        gi_storage.gi_DrInfo  = dri;
        gi = &gi_storage;
    }

    spacing = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
              ? data->ld_HorizSpacing : data->ld_VertSpacing;

    ForeachNode(&data->ld_Children, lc)
    {
        UWORD childW = 0, childH = 0;

        query_child_size(lc, gi, rp, &childW, &childH);

        if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
        {
            totalMin += childW;
            if (childH > maxCross)
                maxCross = childH;
            if (childW > maxMain)
                maxMain = childW;
        }
        else
        {
            totalMin += childH;
            if (childW > maxCross)
                maxCross = childW;
            if (childH > maxMain)
                maxMain = childH;
        }

        count++;
    }

    if (scr)
    {
        if (dri)
            FreeScreenDrawInfo(scr, dri);
        UnlockPubScreen(NULL, scr);
        dri = NULL;
    }

    /* LAYOUT_EvenSize forces every child to the same main-axis size. The
     * minimum that satisfies that constraint is count * largest-natural,
     * which can exceed the simple sum of naturals. Failing to account for
     * this here causes perform_layout's even-share allocation to overflow
     * the available area and push trailing children past the layout edge. */
    if (data->ld_EvenSize && count > 0 && (ULONG)maxMain * count > totalMin)
        totalMin = (ULONG)maxMain * count;

    /* Add spacing between children */
    if (count > 1)
        totalMin += (count - 1) * spacing;

    /* Add outer spacing */
    if (data->ld_SpaceOuter)
    {
        if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
        {
            totalMin += data->ld_LeftSpacing + data->ld_RightSpacing;
            maxCross += data->ld_TopSpacing + data->ld_BottomSpacing;
        }
        else
        {
            totalMin += data->ld_TopSpacing + data->ld_BottomSpacing;
            maxCross += data->ld_LeftSpacing + data->ld_RightSpacing;
        }
    }

    /* Add frame inset for bevel border */
    {
        UWORD t = data->ld_3DLook ? layout_frame_thickness(data->ld_BevelStyle) : 0;
        data->ld_FrameH = t;
        data->ld_FrameV = t;
        if (t)
        {
            totalMin += 2 * t;
            maxCross += 2 * t;
        }
        /* Group-style frame with label needs extra top space.
         * dri is no longer valid here; use a conservative default. */
        if (data->ld_BevelStyle == BVS_GROUP && data->ld_Label)
        {
            UWORD lh = 10;
            if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                maxCross += lh;
            else
                totalMin += lh;
        }
    }

    /* Clamp to UWORD range */
    if (totalMin > 0xFFFF) totalMin = 0xFFFF;
    if (maxCross > 0xFFFF) maxCross = 0xFFFF;

    if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
    {
        data->ld_MinWidth  = (UWORD)totalMin;
        data->ld_MinHeight = (UWORD)maxCross;
    }
    else
    {
        data->ld_MinWidth  = (UWORD)maxCross;
        data->ld_MinHeight = (UWORD)totalMin;
    }

    data->ld_MaxWidth  = 0xFFFF;
    data->ld_MaxHeight = 0xFFFF;
    data->ld_DomainValid = TRUE;
    data->ld_ComputingDomain = FALSE;

    D(bug("[Layout] compute_domain: obj 0x%p min %ldx%ld\n",
        o, (LONG)data->ld_MinWidth, (LONG)data->ld_MinHeight));
}

/******************************************************************************/

void layout_perform_layout(Class *cl, Object *o, struct LayoutData *data,
                           struct GadgetInfo *gi)
{
    struct LayoutChild *lc;
    WORD x, y;
    ULONG availW, availH;
    ULONG totalWeight = 0;
    ULONG count = 0;
    ULONG spacing;
    ULONG outerSpaceW = 0, outerSpaceH = 0;

    x = G(o)->LeftEdge;
    y = G(o)->TopEdge;
    availW = (UWORD)G(o)->Width;
    availH = (UWORD)G(o)->Height;

    /* Apply frame inset (bevel border + label) */
    {
        UWORD t = data->ld_3DLook ? layout_frame_thickness(data->ld_BevelStyle) : 0;
        UWORD topExtra = 0;
        if (data->ld_3DLook && data->ld_BevelStyle == BVS_GROUP && data->ld_Label)
            topExtra = 10;
        if (t || topExtra)
        {
            ULONG insetW = (ULONG)t * 2;
            ULONG insetH = (ULONG)t * 2 + topExtra;
            x += t;
            y += t + topExtra;
            availW = (availW > insetW) ? availW - insetW : 0;
            availH = (availH > insetH) ? availH - insetH : 0;
        }
    }

    /* Apply outer spacing - saturating to avoid UWORD underflow */
    if (data->ld_SpaceOuter)
    {
        outerSpaceW = (ULONG)data->ld_LeftSpacing + data->ld_RightSpacing;
        outerSpaceH = (ULONG)data->ld_TopSpacing  + data->ld_BottomSpacing;
        x += data->ld_LeftSpacing;
        y += data->ld_TopSpacing;
        availW = (availW > outerSpaceW) ? availW - outerSpaceW : 0;
        availH = (availH > outerSpaceH) ? availH - outerSpaceH : 0;
    }

    spacing = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
              ? data->ld_HorizSpacing : data->ld_VertSpacing;

    D(bug("[Layout] perform_layout: obj %p orient=%ld gad=(%d,%d %dx%d) avail=(%d,%d %dx%d) spacing=%d\n",
        o, (LONG)data->ld_Orientation,
        (int)G(o)->LeftEdge, (int)G(o)->TopEdge,
        (int)G(o)->Width,    (int)G(o)->Height,
        (int)x, (int)y, (int)availW, (int)availH, (int)spacing));

    /* Pass 1: count children, sum natural mins on main axis, sum weights */
    {
        ULONG sumMain = 0;
        ULONG maxFloor = 0;
        ForeachNode(&data->ld_Children, lc)
        {
            UWORD nat = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                ? lc->lc_NatWidth : lc->lc_NatHeight;
            UWORD floorMin = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                ? lc->lc_MinWidth : lc->lc_MinHeight;
            if (floorMin < nat) floorMin = nat;

            sumMain += floorMin;
            if (floorMin > maxFloor) maxFloor = floorMin;

            if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                totalWeight += lc->lc_WeightedWidth;
            else
                totalWeight += lc->lc_WeightedHeight;
            count++;
        }

        if (totalWeight == 0) totalWeight = 1;

        /* Remove inter-child spacing from available space */
        if (count > 1)
        {
            ULONG interSpacing = (count - 1) * spacing;
            if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                availW = (availW > interSpacing) ? availW - interSpacing : 0;
            else
                availH = (availH > interSpacing) ? availH - interSpacing : 0;
        }

        /* Compute slack on main axis: avail - sum of floor mins.
         * If avail < sumMain, slack is 0 (children get exactly their floor;
         * layout overflows but children aren't squashed below their natural
         * size). */
        {
            ULONG mainAvail = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ) ? availW : availH;
            ULONG slack = (mainAvail > sumMain) ? mainAvail - sumMain : 0;
            /* When EvenSize is requested, every child must end up the same
             * main-axis size. Allocate a single uniform size here = max(
             * largest natural floor, mainAvail/count) and skip the weighted
             * slack distribution below. */
            ULONG evenSize = 0;
            if (data->ld_EvenSize && count > 0)
            {
                ULONG share = mainAvail / count;
                evenSize = (maxFloor > share) ? maxFloor : share;
            }

            /* Pass 2: assign each child floor + weighted slack */
            ForeachNode(&data->ld_Children, lc)
            {
                ULONG childW, childH;
                UWORD natMain = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                    ? lc->lc_NatWidth : lc->lc_NatHeight;
                UWORD floorMin = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                    ? lc->lc_MinWidth : lc->lc_MinHeight;
                ULONG weight = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                    ? lc->lc_WeightedWidth : lc->lc_WeightedHeight;
                ULONG mainSize;

                if (floorMin < natMain) floorMin = natMain;
                if (data->ld_EvenSize)
                    mainSize = evenSize;
                else
                    mainSize = floorMin + (slack * weight) / totalWeight;

                if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                {
                    childW = mainSize;
                    /* Cross-axis: fill, but cap at MaxHeight/NatHeight when
                     * gadget is image-like and shouldn't stretch. */
                    childH = availH;
                    if (lc->lc_NatHeight && childH > lc->lc_NatHeight
                        && (lc->lc_MaxHeight == 0 || lc->lc_MaxHeight >= lc->lc_NatHeight))
                    {
                        /* Don't stretch a child past its natural height when
                         * no explicit larger MaxHeight requested. This stops
                         * single-row buttons from being squashed/stretched
                         * into oversized rectangles. */
                        if (lc->lc_NatHeight > 0)
                            childH = lc->lc_NatHeight;
                    }
                }
                else
                {
                    childH = mainSize;
                    childW = availW;
                    if (lc->lc_NatWidth && childW > lc->lc_NatWidth
                        && (lc->lc_MaxWidth == 0 || lc->lc_MaxWidth >= lc->lc_NatWidth))
                    {
                        /* Same cross-axis policy for vertical layouts. */
                    }
                }

                /* Enforce explicit min/max constraints */
                if (childW < lc->lc_MinWidth)  childW = lc->lc_MinWidth;
                if (childH < lc->lc_MinHeight) childH = lc->lc_MinHeight;
                if (lc->lc_MaxWidth  && childW > lc->lc_MaxWidth)  childW = lc->lc_MaxWidth;
                if (lc->lc_MaxHeight && childH > lc->lc_MaxHeight) childH = lc->lc_MaxHeight;

                /* Final clamp to UWORD */
                if (childW > 0xFFFF) childW = 0xFFFF;
                if (childH > 0xFFFF) childH = 0xFFFF;

                lc->lc_Left   = x;
                lc->lc_Top    = y;
                lc->lc_Width  = (UWORD)childW;
                lc->lc_Height = (UWORD)childH;

                D(bug("[Layout] perform_layout: obj %p assigning child %p isImage=%d -> (%d,%d) %dx%d (nat=%dx%d weight=%ld)\n",
                    o, lc->lc_Object, (int)lc->lc_IsImage,
                    (int)x, (int)y, (int)childW, (int)childH,
                    (int)lc->lc_NatWidth, (int)lc->lc_NatHeight, (LONG)weight));

                /* Position child object */
                if (lc->lc_Object)
                {
                    if (lc->lc_IsImage)
                    {
                        SetAttrs(lc->lc_Object,
                            IA_Left,   x,
                            IA_Top,    y,
                            IA_Width,  (UWORD)childW,
                            IA_Height, (UWORD)childH,
                            TAG_DONE);
                    }
                    else
                    {
                        struct gpLayout gpl;

                        SetAttrs(lc->lc_Object,
                            GA_Left,   x,
                            GA_Top,    y,
                            GA_Width,  (UWORD)childW,
                            GA_Height, (UWORD)childH,
                            TAG_DONE);

                        /* Give composite gadget children (ClickTab, Page,
                         * nested Layouts, ...) a chance to lay out their
                         * own descendants now that their box is set. */
                        memset(&gpl, 0, sizeof(gpl));
                        gpl.MethodID    = GM_LAYOUT;
                        gpl.gpl_GInfo   = gi;
                        gpl.gpl_Initial = 0;
                        DoMethodA(lc->lc_Object, (Msg)&gpl);
                    }
                }

                if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
                    x += (WORD)childW + spacing;
                else
                    y += (WORD)childH + spacing;
            }
        }
    }
}

/******************************************************************************/

IPTR Layout__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct LayoutData *data = INST_DATA(cl, o);
    struct RastPort *rp;

    D(bug("[Layout] GM_RENDER: obj 0x%p redraw=%ld\n", o, (LONG)msg->gpr_Redraw));

    layout_compute_domain(cl, o, data);
    layout_perform_layout(cl, o, data, msg->gpr_GInfo);

    rp = msg->gpr_RPort;
    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (rp)
    {
        struct LayoutChild *lc;
        struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
        struct Gadget *gad = G(o);

        /* Draw bevel frame if requested. cap_3DLook=FALSE in UIPrefs
         * suppresses the frame. */
        if (data->ld_BevelStyle != BVS_NONE && dri && data->ld_3DLook)
        {
            if (!data->ld_BevelImage)
            {
                data->ld_BevelImage = NewObject(NULL, "bevel.image",
                    BEVEL_Style, data->ld_BevelStyle,
                    data->ld_Label ? BEVEL_Label : TAG_IGNORE,
                    data->ld_Label,
                    TAG_END);
            }
            if (data->ld_BevelImage)
            {
                struct impDraw idmsg;
                idmsg.MethodID         = IM_DRAWFRAME;
                idmsg.imp_RPort        = rp;
                idmsg.imp_Offset.X     = gad->LeftEdge;
                idmsg.imp_Offset.Y     = gad->TopEdge;
                idmsg.imp_State        = IDS_NORMAL;
                idmsg.imp_DrInfo       = dri;
                idmsg.imp_Dimensions.Width  = gad->Width;
                idmsg.imp_Dimensions.Height = gad->Height;
                DoMethodA(data->ld_BevelImage, (Msg)&idmsg);
            }
        }

        /* Render only image children here. Gadget children are linked into
         * the window's GList via LM_ADDTOWINDOW and Intuition refreshes them
         * individually, so rendering them again here would double-draw and
         * fight Intuition's input dispatch. */
        ForeachNode(&data->ld_Children, lc)
        {
            if (lc->lc_Object && lc->lc_IsImage)
            {
                D(bug("[Layout] GM_RENDER: rendering child 0x%p isImage=1 at (%ld,%ld) %ldx%ld\n",
                    lc->lc_Object, (LONG)lc->lc_Left, (LONG)lc->lc_Top,
                    (LONG)lc->lc_Width, (LONG)lc->lc_Height));
                /* perform_layout already wrote absolute coordinates into the
                 * image's IA_Left/IA_Top (im->LeftEdge/TopEdge). Inside IM_DRAW
                 * the class adds imp_Offset to those, so we MUST pass (0,0)
                 * here - otherwise the position is doubled and the label ends
                 * up outside its intended cell, where a sibling's bevel
                 * background fill will overpaint it. */
                DrawImageState(rp, (struct Image *)lc->lc_Object,
                    0, 0,
                    IDS_NORMAL, dri);
            }
        }

        if (!msg->gpr_RPort && msg->gpr_GInfo)
            ReleaseGIRPort(rp);
    }

    return TRUE;
}

/******************************************************************************/

IPTR Layout__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    return GMR_NOREUSE;
}

/******************************************************************************/

IPTR Layout__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    return GMR_NOREUSE;
}

/******************************************************************************/

IPTR Layout__GM_LAYOUT(Class *cl, Object *o, struct gpLayout *msg)
{
    struct LayoutData *data = INST_DATA(cl, o);

    D(bug("[Layout] GM_LAYOUT: obj 0x%p box (%ld,%ld) %ldx%ld\n",
        o, (LONG)G(o)->LeftEdge, (LONG)G(o)->TopEdge,
        (LONG)G(o)->Width, (LONG)G(o)->Height));

    data->ld_DomainValid = FALSE;
    layout_compute_domain(cl, o, data);
    layout_perform_layout(cl, o, data, msg->gpl_GInfo);

    D(bug("[Layout] GM_LAYOUT: domain min %ldx%ld\n",
        (LONG)data->ld_MinWidth, (LONG)data->ld_MinHeight));

    return TRUE;
}

/******************************************************************************/

IPTR Layout__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct LayoutData *data = INST_DATA(cl, o);

    layout_compute_domain(cl, o, data);

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = data->ld_MinWidth;
    msg->gpd_Domain.Height = data->ld_MinHeight;

    return TRUE;
}

/******************************************************************************/

IPTR Layout__GM_HITTEST(Class *cl, Object *o, struct gpHitTest *msg)
{
    /* The layout itself never claims a hit. Child gadgets are added directly
     * to the window's GList by LM_ADDTOWINDOW, so Intuition hit-tests them
     * itself. Returning GMR_GADGETHIT here would cause the layout to swallow
     * input intended for the children. */
    return 0;
}

/******************************************************************************/

static void layout_collect_gadgets(Class *cl, Object *o,
                                   struct Gadget **head,
                                   struct Gadget **tail,
                                   ULONG *count)
{
    struct LayoutData *data = INST_DATA(cl, o);
    struct LayoutChild *lc;
    struct Gadget *g;

    /* Append the layout itself first so Intuition draws its background/bevel
     * before children paint over it. */
    g = (struct Gadget *)o;
    g->NextGadget = NULL;
    if (*tail)
        (*tail)->NextGadget = g;
    else
        *head = g;
    *tail = g;
    (*count)++;

    ForeachNode(&data->ld_Children, lc)
    {
        if (!lc->lc_Object || lc->lc_IsImage)
            continue;

        if (OCLASS(lc->lc_Object) == cl)
        {
            /* Nested layout - recurse */
            layout_collect_gadgets(cl, lc->lc_Object, head, tail, count);
        }
        else
        {
            /* Leaf gadget OR a non-layout container (e.g. page.gadget).
             * Container children's inner gadgets are added separately
             * via their own LM_ADDTOWINDOW handler from
             * Layout__LM_ADDTOWINDOW after this chain is linked. */
            g = (struct Gadget *)lc->lc_Object;
            g->NextGadget = NULL;
            if (*tail)
                (*tail)->NextGadget = g;
            else
                *head = g;
            *tail = g;
            (*count)++;
        }
    }
}

/******************************************************************************/

/* Walk the layout subtree and forward LM_ADDTOWINDOW to any non-layout
 * child gadget. Gadgets that don't implement the method (rootclass
 * fallback returns 0) are silently ignored. This is what wires up
 * page.gadget's children when a page is embedded inside a layout. */
static void layout_forward_addtowindow(Class *cl, Object *o,
                                       struct Window *win,
                                       struct Requester *req)
{
    struct LayoutData *data = INST_DATA(cl, o);
    struct LayoutChild *lc;
    struct lpAddToWindow lpa;

    lpa.MethodID       = LM_ADDTOWINDOW;
    lpa.lpaw_Window    = win;
    lpa.lpaw_Requester = req;

    ForeachNode(&data->ld_Children, lc)
    {
        if (!lc->lc_Object || lc->lc_IsImage)
            continue;

        if (OCLASS(lc->lc_Object) == cl)
        {
            /* Nested layout: descend so any embedded containers in the
             * subtree also get their LM_ADDTOWINDOW invoked. */
            layout_forward_addtowindow(cl, lc->lc_Object, win, req);
        }
        else
        {
            /* Non-layout container or leaf gadget. If the class handles
             * LM_ADDTOWINDOW it will add its own children; if not, the
             * call is a harmless no-op (returns 0 from rootclass). */
            DoMethodA(lc->lc_Object, (Msg)&lpa);
        }
    }
}

/******************************************************************************/

IPTR Layout__LM_ADDTOWINDOW(Class *cl, Object *o, struct lpAddToWindow *msg)
{
    struct Gadget *head = NULL, *tail = NULL;
    ULONG count = 0;

    layout_collect_gadgets(cl, o, &head, &tail, &count);

    D(bug("[Layout] LM_ADDTOWINDOW: obj 0x%p win=0x%p chain head=0x%p count=%ld\n",
        o, msg->lpaw_Window, head, (LONG)count));

    if (head && msg->lpaw_Window && count)
    {
        AddGList(msg->lpaw_Window, head, (UWORD)-1, count, msg->lpaw_Requester);
        RefreshGList(head, msg->lpaw_Window, msg->lpaw_Requester, count);
    }

    /* After our own chain is linked, give any non-layout container
     * children (e.g. page.gadget) a chance to add their own gadgets. */
    if (msg->lpaw_Window)
        layout_forward_addtowindow(cl, o, msg->lpaw_Window, msg->lpaw_Requester);

    return count;
}

/******************************************************************************/

IPTR Layout__LM_REMOVEFROMWINDOW(Class *cl, Object *o, struct lpAddToWindow *msg)
{
    struct Gadget *g = (struct Gadget *)o;
    struct Gadget *next;
    ULONG count = 0;

    /* Symmetric to LM_ADDTOWINDOW: first ask any non-layout container
     * children to remove their own gadgets. */
    {
        struct LayoutData *data = INST_DATA(cl, o);
        struct LayoutChild *lc;
        struct lpAddToWindow lpr;

        lpr.MethodID       = LM_REMOVEFROMWINDOW;
        lpr.lpaw_Window    = msg->lpaw_Window;
        lpr.lpaw_Requester = msg->lpaw_Requester;

        ForeachNode(&data->ld_Children, lc)
        {
            if (!lc->lc_Object || lc->lc_IsImage) continue;
            if (OCLASS(lc->lc_Object) != cl)
                DoMethodA(lc->lc_Object, (Msg)&lpr);
        }
    }

    /* Count the chain we previously linked (starts at the layout itself). */
    for (next = g; next; next = next->NextGadget)
        count++;

    D(bug("[Layout] LM_REMOVEFROMWINDOW: obj 0x%p win=0x%p count=%ld\n",
        o, msg->lpaw_Window, (LONG)count));

    if (msg->lpaw_Window && count)
        RemoveGList(msg->lpaw_Window, g, count);

    /* Tear the flat chain down so reopening rebuilds cleanly. */
    next = g;
    while (next)
    {
        struct Gadget *n = next->NextGadget;
        next->NextGadget = NULL;
        next = n;
    }

    return count;
}
