/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction layout.gadget - BOOPSI class implementation
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
#include <gadgets/layout.h>
#include <utility/tagitem.h>

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
                data->ld_BevelStyle = tag->ti_Data;
                break;

            case LAYOUT_Alignment:
                data->ld_Alignment = tag->ti_Data;
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
                data->ld_Label = (STRPTR)tag->ti_Data;
                break;

            case LAYOUT_LabelPlace:
                data->ld_LabelPlace = tag->ti_Data;
                break;

            case LAYOUT_HorizSpacing:
                data->ld_HorizSpacing = (UWORD)tag->ti_Data;
                break;

            case LAYOUT_VertSpacing:
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

            case LAYOUT_RemoveImage:
            {
                Object *img = (Object *)tag->ti_Data;
                struct LayoutChild *node, *next;

                ForeachNodeSafe(&data->ld_Children, node, next)
                {
                    if (node->lc_Object == img && node->lc_IsImage)
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

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct LayoutData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct LayoutData));
        NewList((struct List *)&data->ld_Children);

        /* Defaults */
        data->ld_Orientation = LAYOUT_ORIENT_HORIZ;
        data->ld_SpaceInner  = TRUE;
        data->ld_HorizSpacing = 4;
        data->ld_VertSpacing  = 2;
        data->ld_TopSpacing   = 4;
        data->ld_BottomSpacing = 4;
        data->ld_LeftSpacing  = 4;
        data->ld_RightSpacing = 4;

        layout_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Layout__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct LayoutData *data = INST_DATA(cl, o);
    struct LayoutChild *lc, *next;

    ForeachNodeSafe(&data->ld_Children, lc, next)
    {
        Remove((struct Node *)lc);
        free_child(lc);
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Layout__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
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

void layout_compute_domain(Class *cl, Object *o, struct LayoutData *data)
{
    struct LayoutChild *lc;
    UWORD totalMin = 0;
    UWORD maxCross = 0;
    UWORD count = 0;
    UWORD spacing;

    if (data->ld_DomainValid)
        return;

    spacing = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
              ? data->ld_HorizSpacing : data->ld_VertSpacing;

    ForeachNode(&data->ld_Children, lc)
    {
        UWORD childMinW = lc->lc_MinWidth;
        UWORD childMinH = lc->lc_MinHeight;

        /* Query gadget minimum if not explicitly set */
        if (childMinW == 0 && lc->lc_Object && !lc->lc_IsImage)
            childMinW = 40;
        if (childMinH == 0 && lc->lc_Object && !lc->lc_IsImage)
            childMinH = 14;

        if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
        {
            totalMin += childMinW;
            if (childMinH > maxCross)
                maxCross = childMinH;
        }
        else
        {
            totalMin += childMinH;
            if (childMinW > maxCross)
                maxCross = childMinW;
        }

        count++;
    }

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

    if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
    {
        data->ld_MinWidth  = totalMin;
        data->ld_MinHeight = maxCross;
    }
    else
    {
        data->ld_MinWidth  = maxCross;
        data->ld_MinHeight = totalMin;
    }

    data->ld_MaxWidth  = 0xFFFF;
    data->ld_MaxHeight = 0xFFFF;
    data->ld_DomainValid = TRUE;
}

/******************************************************************************/

void layout_perform_layout(Class *cl, Object *o, struct LayoutData *data,
                           struct GadgetInfo *gi)
{
    struct LayoutChild *lc;
    WORD x, y;
    UWORD availW, availH;
    UWORD totalWeight = 0;
    UWORD count = 0;
    UWORD spacing;

    x = G(o)->LeftEdge;
    y = G(o)->TopEdge;
    availW = G(o)->Width;
    availH = G(o)->Height;

    /* Apply outer spacing */
    if (data->ld_SpaceOuter)
    {
        x += data->ld_LeftSpacing;
        y += data->ld_TopSpacing;
        availW -= data->ld_LeftSpacing + data->ld_RightSpacing;
        availH -= data->ld_TopSpacing + data->ld_BottomSpacing;
    }

    spacing = (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
              ? data->ld_HorizSpacing : data->ld_VertSpacing;

    /* Count children and total weights */
    ForeachNode(&data->ld_Children, lc)
    {
        if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
            totalWeight += lc->lc_WeightedWidth;
        else
            totalWeight += lc->lc_WeightedHeight;
        count++;
    }

    if (totalWeight == 0) totalWeight = 1;

    /* Remove spacing from available space */
    if (count > 1)
    {
        if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
            availW -= (count - 1) * spacing;
        else
            availH -= (count - 1) * spacing;
    }

    /* Layout children */
    ForeachNode(&data->ld_Children, lc)
    {
        UWORD childW, childH;

        if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
        {
            if (data->ld_EvenSize)
                childW = availW / count;
            else
                childW = (availW * lc->lc_WeightedWidth) / totalWeight;
            childH = availH;
        }
        else
        {
            childW = availW;
            if (data->ld_EvenSize)
                childH = availH / count;
            else
                childH = (availH * lc->lc_WeightedHeight) / totalWeight;
        }

        /* Apply min/max constraints */
        if (childW < lc->lc_MinWidth) childW = lc->lc_MinWidth;
        if (childH < lc->lc_MinHeight) childH = lc->lc_MinHeight;
        if (childW > lc->lc_MaxWidth) childW = lc->lc_MaxWidth;
        if (childH > lc->lc_MaxHeight) childH = lc->lc_MaxHeight;

        lc->lc_Left   = x;
        lc->lc_Top    = y;
        lc->lc_Width  = childW;
        lc->lc_Height = childH;

        /* Position child object */
        if (lc->lc_Object)
        {
            if (lc->lc_IsImage)
            {
                /* Images use IA_Left/IA_Top/IA_Width/IA_Height */
                SetAttrs(lc->lc_Object,
                    IA_Left,   x,
                    IA_Top,    y,
                    IA_Width,  childW,
                    IA_Height, childH,
                    TAG_DONE);
            }
            else
            {
                /* Gadgets use GA_Left/GA_Top/GA_Width/GA_Height */
                SetAttrs(lc->lc_Object,
                    GA_Left,   x,
                    GA_Top,    y,
                    GA_Width,  childW,
                    GA_Height, childH,
                    TAG_DONE);
            }
        }

        /* Advance position */
        if (data->ld_Orientation == LAYOUT_ORIENT_HORIZ)
            x += childW + spacing;
        else
            y += childH + spacing;
    }
}

/******************************************************************************/

IPTR Layout__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct LayoutData *data = INST_DATA(cl, o);
    struct RastPort *rp;

    layout_compute_domain(cl, o, data);
    layout_perform_layout(cl, o, data, msg->gpr_GInfo);

    rp = msg->gpr_RPort;
    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (rp)
    {
        struct LayoutChild *lc;

        /* Render each child */
        ForeachNode(&data->ld_Children, lc)
        {
            if (lc->lc_Object)
            {
                if (lc->lc_IsImage)
                {
                    DrawImageState(rp, (struct Image *)lc->lc_Object,
                        lc->lc_Left, lc->lc_Top,
                        IDS_NORMAL, msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL);
                }
                else
                {
                    DoMethod(lc->lc_Object, GM_RENDER, (IPTR)msg->gpr_GInfo,
                             (IPTR)rp, (IPTR)msg->gpr_Redraw);
                }
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

    data->ld_DomainValid = FALSE;
    layout_compute_domain(cl, o, data);
    layout_perform_layout(cl, o, data, msg->gpl_GInfo);

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
    struct LayoutData *data = INST_DATA(cl, o);
    struct LayoutChild *lc;

    ForeachNode(&data->ld_Children, lc)
    {
        if (lc->lc_Object && !lc->lc_IsImage)
        {
            if (msg->gpht_Mouse.X >= lc->lc_Left - G(o)->LeftEdge &&
                msg->gpht_Mouse.X < lc->lc_Left - G(o)->LeftEdge + lc->lc_Width &&
                msg->gpht_Mouse.Y >= lc->lc_Top - G(o)->TopEdge &&
                msg->gpht_Mouse.Y < lc->lc_Top - G(o)->TopEdge + lc->lc_Height)
            {
                return GMR_GADGETHIT;
            }
        }
    }

    return 0;
}
