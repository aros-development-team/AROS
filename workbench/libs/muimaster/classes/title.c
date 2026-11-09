/*
    Copyright (C) 2012-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "title_private.h"

/*  #define MYDEBUG 1 */
#include "debug.h"

/****** Title.mui/MUIC_Title ************
*
*   NAME
*       MUIC_Title
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Display a horizontal row of child objects as tab titles and select the
*       corresponding page in the parent Group. The public Title header defines
*       only the class name: there are no Title-specific public attributes or
*       methods. Child objects, their contents and their disposal follow Group
*       ownership rules.
*       For the tabbed-page arrangement implemented by AROS Group, use a parent
*       with MUIA_Frame set to MUIV_Frame_Register and MUIA_Group_PageMode TRUE.
*       Supply Title as its first child and the page objects afterward, in the
*       same order as the title children. Group treats the first child as the
*       persistent title group and excludes it from page numbering. Provide at
*       least one title and one corresponding page. Title does not create labels
*       or pages and does not validate their correspondence.
*       Construction installs a private layout hook, horizontal grouping and
*       ShowSelState FALSE before caller tags. Preserve the layout hook for this
*       arrangement. Location is fixed to the top in the constructor. Internal
*       left-side drawing/layout branches do not constitute a public orientation
*       API.
*
*   METHODS
*       On a changed MUIA_Group_ActivePage tag, Title saves its old tab index,
*       stores the raw new value, requests an update redraw and sets the
*       parent's ActivePage before delegating to Group. A live parent is
*       required for that path. Title starts its own active-tab cache at zero;
*       it does not initialize that cache from constructor ActivePage tags and
*       has no getter for the cache. An inherited ActivePage query is not a
*       query of this private state.
*       Use valid zero-based indices matching title and page order. Set the
*       parent Group's ActivePage to use its First, Last, Prev, Next or Advance
*       interpretation; Title itself does not normalize sentinel values or check
*       ranges. Parent Group feeds a changed, resolved page back to Title. Do
*       not rely on an invalid index being rejected or on a raw sentinel sent to
*       Title leaving its cache coherent.
*       After successful superclass Setup, Title snapshots the parent background
*       and registers an IDCMP_MOUSEBUTTONS handler with GUI mode and priority
*       -7, so contained controls with higher priority can handle input first.
*       SELECTDOWN inside a title child's rectangle selects that child's index.
*       Padding outside child rectangles does not select a tab. The handler
*       returns zero even after selecting; there is no local keyboard-selection
*       handler.
*       Cleanup removes the event handler before delegating. Disposal frees the
*       owned layout hook before delegating child disposal. Follow the normal
*       Setup/Cleanup/disposal lifecycle; do not dispose during callbacks or
*       drawing.
*
*   NOTES
*       Top layout divides available width among all children after horizontal
*       spacing, distributing remaining pixels to the earliest children. Each
*       child is inset four pixels horizontally and vertically. Minimum/default
*       width is child count times the largest child minimum width plus child
*       count times horizontal spacing. Height is fixed to the largest child
*       minimum height plus eight. Calculations use WORD intermediates and do
*       not ensure the width includes the eight-pixel child inset; provide
*       usable, nonoverflowing geometry.
*       Drawing delegates to Group, then draws active and previously active
*       frames for an update, or all frames for a full draw. Active children
*       receive the parent background captured at Setup; inactive children
*       receive MUII_BACKGROUND. These background sets suppress notification but
*       can request redraw. Later parent background changes are not
*       automatically recaptured.
*
*   BUGS
*       An empty title group is unsafe: top layout divides by the child count
*       and drawing dereferences the first child without checking for NULL. Do
*       not remove the last child while the group can be laid out or drawn.
*       If superclass construction fails after allocating the layout hook, the
*       constructor returns failure without freeing that hook.
*       The conditional builtin dispatcher spells the disposal call
*       Title_OM_DISPOSE, whereas the definition in this file is
*       Title__OM_DISPOSE. The external class method list declares OM_DISPOSE
*       separately. This source discrepancy is not corrected here, and neither
*       build configuration nor runtime behavior has been validated by this
*       documentation change.
*
*/

extern struct Library *MUIMasterBase;

/* Note: Code taken from OWB's tabs handling */

#define MUIB_Tabs                   (TAG_USER | 0x10000000)
#define MUIV_Tabs_Top               (MUIB_Tabs | 0x00000000)
#define MUIV_Tabs_Left              (MUIB_Tabs | 0x00000001)

#define _isinobj(x,y,obj) (_between(_left(obj),(x),_right (obj)) \
                          && _between(_top(obj) ,(y),_bottom(obj)))

#define VERT_PADDING    (4)
#define HORIZ_PADDING   (4)

/*
 * Use the borrowed layout message and live hook instance to calculate
 * min/max or place children. Top mode requires nonzero child count and sane
 * WORD geometry; divides remaining width equally and insets children by
 * four pixels. Return zero for MINMAX, FALSE on layout failure/unknown
 * location, otherwise TRUE. No ownership transfer or rollback of already
 * placed children.
 */
ULONG Tabs_Layout_Function(struct Hook *hook, Object *obj,
    struct MUI_LayoutMsg *lm)
{
    struct Title_DATA *data = (struct Title_DATA *)hook->h_Data;
    switch (lm->lm_Type)
    {
    case MUILM_MINMAX:
        {
            Object *cstate = (Object *) lm->lm_Children->mlh_Head;
            Object *child;

            WORD maxminwidth = 0;
            WORD maxminheight = 0;
            WORD mintotalwidth = 0;
            WORD mintotalheight = 0;
            LONG number_of_children = 0;

            /* find out biggest widths & heights of our children */

            while ((child = NextObject(&cstate)))
            {
                if (maxminwidth < MUI_MAXMAX
                    && _minwidth(child) > maxminwidth)
                    maxminwidth = _minwidth(child);

                if (maxminheight < MUI_MAXMAX
                    && _minheight(child) > maxminheight)
                    maxminheight = _minheight(child);

                mintotalheight += _minheight(child);

                number_of_children++;
            }

            if (data->location == MUIV_Tabs_Top)
            {
                WORD horiz_spacing = XGET(obj, MUIA_Group_HorizSpacing);

                mintotalwidth = (number_of_children * maxminwidth) +
                        ((number_of_children - 1) * horiz_spacing) + horiz_spacing;

                lm->lm_MinMax.MinWidth = lm->lm_MinMax.DefWidth = mintotalwidth;
                lm->lm_MinMax.MaxWidth = MUI_MAXMAX;

                lm->lm_MinMax.MinHeight =
                lm->lm_MinMax.MaxHeight =
                lm->lm_MinMax.DefHeight = maxminheight + (VERT_PADDING * 2);
            }
            else if (data->location == MUIV_Tabs_Left)
            {
                mintotalheight +=
                    (number_of_children - 1) * XGET(obj,
                    MUIA_Group_VertSpacing);
                lm->lm_MinMax.MinWidth = lm->lm_MinMax.DefWidth =
                    maxminwidth;
                lm->lm_MinMax.MinHeight = lm->lm_MinMax.DefHeight =
                    mintotalheight;
                lm->lm_MinMax.MaxWidth = lm->lm_MinMax.DefWidth =
                    maxminwidth;
                lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
            }

            return 0;
        }
    case MUILM_LAYOUT:
        {
            APTR cstate;
            Object *child;
            LONG number_of_children = 0;

            cstate = lm->lm_Children->mlh_Head;
            while ((child = NextObject(&cstate)))
            {
                number_of_children++;
            }

            if (data->location == MUIV_Tabs_Top)
            {
                WORD horiz_spacing = XGET(obj, MUIA_Group_HorizSpacing);
                WORD childwidth =
                        (lm->lm_Layout.Width - ((number_of_children - 1) * horiz_spacing) - horiz_spacing)
                        / number_of_children;

                WORD leftovers = lm->lm_Layout.Width -
                        ((number_of_children - 1) * horiz_spacing) - horiz_spacing -
                        (number_of_children * childwidth);

                WORD left = horiz_spacing / 2;
                WORD cheight = _height(obj) - (VERT_PADDING * 2);

                cstate = lm->lm_Children->mlh_Head;
                while ((child = NextObject(&cstate)))
                {
                    WORD cwidth = childwidth;

                    if (leftovers-- > 0)
                        cwidth++;
                    if (!MUI_Layout(child, left + HORIZ_PADDING, VERT_PADDING, cwidth - (HORIZ_PADDING * 2), cheight , 0))
                        return (FALSE);

                    left += cwidth + horiz_spacing;
                }
            }
            else if (data->location == MUIV_Tabs_Left)
            {
                WORD vert_spacing = XGET(obj, MUIA_Group_VertSpacing);
                WORD top = 0;
                cstate = lm->lm_Children->mlh_Head;
                while ((child = NextObject(&cstate)))
                {
                    WORD cheight = _minheight(child);

                    if (!MUI_Layout(child, 0, top, lm->lm_Layout.Width,
                            cheight, 0))
                        return (FALSE);

                    top += cheight + vert_spacing;
                }
            }
            else
                return FALSE;

            return TRUE;
        }
    }

    return TRUE;
}

/*
 * Allocate owned layout hook, create Group with horizontal/layout defaults,
 * then bind hook data and initialize active cache zero and mouse handler.
 * Return object or zero. Superclass failure leaks allocated hook. Caller
 * must supply nonempty matching title children and later a live page-mode
 * parent; constructor tags do not initialize the private active cache.
 */
IPTR Title__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Title_DATA *data = NULL;
    struct Hook *layout_hook;

    /* Lay tabs horizontally by default */
    LONG location = MUIV_Tabs_Top;

    layout_hook = AllocVec(sizeof(struct Hook), MEMF_ANY | MEMF_CLEAR);
    if (!layout_hook)
        return (IPTR) NULL;

    layout_hook->h_Entry = HookEntry;
    layout_hook->h_SubEntry = (HOOKFUNC) Tabs_Layout_Function;

    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_LayoutHook, (IPTR) layout_hook,
        MUIA_ShowSelState, FALSE, TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    layout_hook->h_Data = data;
    data->layout_hook = layout_hook;
    data->location = location;
    data->activetab = 0;
    data->oldactivetab = 0;

    /* We need tab events to be processed after all objects contained in tabs
       like for example close button, hence the low priority value */
    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS;
    data->ehn.ehn_Priority = -7;
    data->ehn.ehn_Flags = MUI_EHF_GUIMODE;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    D(bug("muimaster.library/title.c: Title Object created at 0x%p\n",
            obj));

    return (IPTR) obj;
}

/*
 * Free the instance-owned layout hook then delegate Group disposal and
 * return its result. Requires completed Cleanup, no
 * callbacks/layout/drawing in progress and valid instance. Child ownership
 * follows Group; this handler does not dispose the parent.
 */
IPTR Title__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);
    FreeVec(data->layout_hook);

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * For changed ActivePage, store raw value and old cache, redraw and set
 * live parent before delegating Group with the original message. Return
 * superclass result, not selection validation. No range/sentinel
 * normalization or private getter; callbacks must preserve instance and
 * parent.
 */
IPTR Title__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Group_ActivePage:
            {
                if (data->activetab != tag->ti_Data)
                {
                    data->oldactivetab = data->activetab;
                    data->activetab = tag->ti_Data;
                    MUI_Redraw(obj, MADF_DRAWUPDATE);
                    set(_parent(obj), MUIA_Group_ActivePage,
                        data->activetab);
                }
                break;
            }
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/* MUIM_Draw helpers */

/* Similar like in Register class */
/*
 * Draw top tab padding and rounded frame using borrowed child render
 * context, setup pens and valid WORD bounds. Adjust bounds for fixed
 * padding; active flag selects lower border shape. Return void, retain no
 * resources and leave drawing pen changed. Requires drawable child and
 * valid parent background operations.
 */
static void DrawTopTab(Object *obj, struct Title_DATA *data, BOOL active,
    WORD x1, WORD y1, WORD x2, WORD y2)
{
    /* Correct for padding */
    x1 -= HORIZ_PADDING;
    y1 -= VERT_PADDING;
    x2 += HORIZ_PADDING;
    y2 += 1;

    /* Fill the padding area */
    DoMethod(obj, MUIM_DrawBackground, x1 , y1, x2 - x1, VERT_PADDING, 0, 0, 0);
    DoMethod(obj, MUIM_DrawBackground, x1 , y1, HORIZ_PADDING, y2 - y1, 0, 0, 0);
    DoMethod(obj, MUIM_DrawBackground, x2 - VERT_PADDING + 1, y1, HORIZ_PADDING, y2 - y1, 0, 0, 0);
    DoMethod(obj, MUIM_DrawBackground, x1, y2, x2 - x1, 1, 0, 0, 0);

    if (!active)
    {
        /* Clear the rounded edges of an inactive tab with default
         * background */

        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1, (IPTR) y1,
            (IPTR) 1, (IPTR) 3, (IPTR) x1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1 + 1, (IPTR) y1,
            (IPTR) 1, (IPTR) 2, (IPTR) x1 + 1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1 + 2, (IPTR) y1,
            (IPTR) 1, (IPTR) 1, (IPTR) x1 + 2, (IPTR) y1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x2 - 3, (IPTR) y1,
            (IPTR) 3, (IPTR) 1, (IPTR) x2 - 3, (IPTR) y1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x2 - 1, (IPTR) y1,
            (IPTR) 1, (IPTR) 2, (IPTR) x2 - 1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x2, (IPTR) y1,
            (IPTR) 1, (IPTR) 3, (IPTR) x2, (IPTR) y1, (IPTR) 0);
    }

    /* top horiz bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    RectFill(_rp(obj), x1 + 4, y1, x2 - 4, y1);

    /* left vert bar */
    RectFill(_rp(obj), x1, y1 + 4, x1, y2 - (active ? 2 : 1));
    WritePixel(_rp(obj), x1 + 1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 1);
    WritePixel(_rp(obj), x1 + 3, y1 + 1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHINE]);
    WritePixel(_rp(obj), x1 + 3, y1);
    WritePixel(_rp(obj), x1 + 4, y1 + 1);
    WritePixel(_rp(obj), x1 + 2, y1 + 2);
    WritePixel(_rp(obj), x1 + 3, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 3);
    WritePixel(_rp(obj), x1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 4);

    if (active)
    {
        /* bottom horiz bar */
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        WritePixel(_rp(obj), x1, y2 - 1);
        WritePixel(_rp(obj), x1, y2);

        SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
        WritePixel(_rp(obj), x2, y2 - 1);
        WritePixel(_rp(obj), x2, y2);
    }
    else
    {
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        RectFill(_rp(obj), x1, y2, x2, y2);
    }

    /* right vert bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    WritePixel(_rp(obj), x2 - 1, y1 + 2);
    RectFill(_rp(obj), x2, y1 + 4, x2, y2 - (active ? 2 : 1));
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);
    WritePixel(_rp(obj), x2 - 2, y1 + 1);
    WritePixel(_rp(obj), x2 - 1, y1 + 3);
    WritePixel(_rp(obj), x2, y1 + 3);
    SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    WritePixel(_rp(obj), x2 - 3, y1 + 1);
}

/* Like above, just symetrically flipped */
/*
 * Draw the internal left-tab frame using borrowed drawable child context,
 * setup pens and valid WORD bounds. Return void without resource ownership;
 * leaves pen changed. This orientation has no public selector in this
 * implementation, but its helper still requires a live parent for
 * background drawing.
 */
static void DrawLeftTab(Object *obj, struct Title_DATA *data, BOOL active,
    WORD x1, WORD y1, WORD x2, WORD y2)
{
    if (!active)
    {
        /* Clear the rounded edges of an inactive tab with default
         * background */

        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1, (IPTR) y1,
            (IPTR) 3, (IPTR) 1, (IPTR) x1, (IPTR) y1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1, (IPTR) y1 + 1,
            (IPTR) 2, (IPTR) 1, (IPTR) x1, (IPTR) y1 + 1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1, (IPTR) y1 + 2,
            (IPTR) 1, (IPTR) 1, (IPTR) x1, (IPTR) y1 + 2, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1, (IPTR) y2 - 2,
            (IPTR) 1, (IPTR) 1, (IPTR) x1, (IPTR) y2 - 2, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1, (IPTR) y2 - 1,
            (IPTR) 2, (IPTR) 1, (IPTR) x1, (IPTR) y2 - 1, (IPTR) 0);
        DoMethod(obj, MUIM_DrawParentBackground, (IPTR) x1, (IPTR) y2,
            (IPTR) 3, (IPTR) 1, (IPTR) x1, (IPTR) y2, (IPTR) 0);

    }

    /* left vert bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    RectFill(_rp(obj), x1, y1 + 4, x1, y2 - 4);

    /* top horiz bar */
    RectFill(_rp(obj), x1 + 4, y1, x2 - (active ? 2 : 1), y1);
    WritePixel(_rp(obj), x1 + 1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 1);
    WritePixel(_rp(obj), x1 + 3, y1 + 1);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHINE]);
    WritePixel(_rp(obj), x1 + 3, y1);
    WritePixel(_rp(obj), x1 + 4, y1 + 1);
    WritePixel(_rp(obj), x1 + 2, y1 + 2);
    WritePixel(_rp(obj), x1 + 3, y1 + 2);
    WritePixel(_rp(obj), x1 + 2, y1 + 3);
    WritePixel(_rp(obj), x1, y1 + 3);
    WritePixel(_rp(obj), x1 + 1, y1 + 4);

    if (active)
    {
        /* bottom horiz bar */
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        WritePixel(_rp(obj), x2 - 1, y1);
        WritePixel(_rp(obj), x2, y1);

        SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
        WritePixel(_rp(obj), x2 - 1, y2);
        WritePixel(_rp(obj), x2, y2);
    }
    else
    {
        SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
        RectFill(_rp(obj), x2, y1, x2, y2);
    }

    /* bottom horiz bar */
    SetAPen(_rp(obj), _pens(obj)[MPEN_SHADOW]);
    WritePixel(_rp(obj), x1 + 2, y2 - 1);
    RectFill(_rp(obj), x1 + 4, y2, x2 - (active ? 2 : 1), y2);
    SetAPen(_rp(obj), _pens(obj)[MPEN_HALFSHADOW]);
    WritePixel(_rp(obj), x1 + 1, y2 - 2);
    WritePixel(_rp(obj), x1 + 3, y2 - 1);
    WritePixel(_rp(obj), x1 + 3, y2);
    SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);
    WritePixel(_rp(obj), x1 + 1, y2 - 3);
}

/* Drawing code */
/*
 * Set child background without notification to cached parent background
 * when active, otherwise MUII_BACKGROUND; draw the frame for stored
 * internal location. Requires live drawable child and valid
 * instance/background storage. Return TRUE even for unrecognized location;
 * background changes can schedule redraw, no resources transferred.
 */
IPTR Tab__MUIM_Draw(Object *child, struct Title_DATA *data, BOOL active)
{
    WORD x1 = _left(child);
    WORD y1 = _top(child);
    WORD x2 = _right(child);
    WORD y2 = _bottom(child);

    /* Setting of background (to a different value!) causes redraw of object. We use this "feature" */
    if (active)
        nnset(child, MUIA_Background, data->background);
    else
        nnset(child, MUIA_Background, MUII_BACKGROUND);

    /* Draw tab frame */
    if (data->location == MUIV_Tabs_Top)
        DrawTopTab(child, data, active, x1, y1, x2, y2);
    else if (data->location == MUIV_Tabs_Left)
        DrawLeftTab(child, data, active, x1, y1, x2, y2);

    return TRUE;
}
/* MUIM_Draw helpers */

/*
 * Delegate drawing to Group then update active/old frames or all frames on
 * full draw and fill border gaps. Requires nonempty child list, valid setup
 * render context and usable geometry; top path dereferences first child
 * without NULL check. Return zero for irrelevant flags, FALSE for unknown
 * location, otherwise TRUE. Background sets can schedule child redraw.
 */
IPTR Title__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);
    struct List *children = NULL;
    APTR cstate;
    Object *child;
    WORD horiz_spacing = XGET(obj, MUIA_Group_HorizSpacing);
    WORD vert_spacing = XGET(obj, MUIA_Group_VertSpacing);
    LONG tab = 0;

    /* Draw all the children */
    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return (0);


    /* Now draw missing TabbedGroup border between the spaces */
    get(obj, MUIA_Group_ChildList, &children);

    cstate = children->lh_Head;

    while ((child = NextObject(&cstate)))
    {
        if (tab == data->activetab)
            Tab__MUIM_Draw(child, data, TRUE);
        else if (tab == data->oldactivetab)
            Tab__MUIM_Draw(child, data, FALSE);
        else if (msg->flags & MADF_DRAWOBJECT)
            Tab__MUIM_Draw(child, data, FALSE);

        tab++;
    }

    cstate = children->lh_Head;
    child = NextObject(&cstate);

    SetAPen(_rp(obj), _pens(obj)[MPEN_SHINE]);
    if (data->location == MUIV_Tabs_Top)
    {
        RectFill(_rp(obj), _left(obj), _bottom(child) + 1, _left(obj) + horiz_spacing / 2, _bottom(child) + 1);
        RectFill(_rp(obj), _right(obj) - horiz_spacing / 2, _bottom(child) + 1, _right(obj), _bottom(child) + 1);

        while (child && (child = NextObject(&cstate)))
        {
            RectFill(_rp(obj), _left(child) - horiz_spacing - HORIZ_PADDING, _bottom(child) + 1,
                _left(child) - 1 - HORIZ_PADDING, _bottom(child) + 1);
        }
    }
    else if (data->location == MUIV_Tabs_Left)
    {
        WORD lasty = -1;
        while (child && (child = NextObject(&cstate)))
        {
            RectFill(_rp(obj), _right(child), _top(child) - vert_spacing,
                _right(child), _top(child) - 1);
            lasty = _bottom(child);
        }
        if (lasty > -1)
            RectFill(_rp(obj), _right(obj), lasty + 1, _right(obj),
                _bottom(obj));
    }
    else
        return FALSE;

    return TRUE;
}

/*
 * Require successful Group Setup, snapshot live parent Background without
 * checking getter result, then register instance mouse event handler on
 * live Window. Return FALSE on superclass failure, otherwise TRUE without
 * checking handler-registration result. Pair successful setup with Cleanup;
 * borrowed parent background must remain usable.
 */
IPTR Title__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    get(_parent(obj), MUIA_Background, &data->background);

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    return TRUE;
}

/*
 * Remove instance event handler from the live Window before delegating
 * Group Cleanup and returning its result. Requires matching successful
 * Setup and no concurrent dispatch; does not free the layout hook or parent
 * background.
 */
IPTR Title__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Title_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * For a non-NULL SELECTDOWN mouse message inside this Title, find child
 * rectangle and Set ActivePage to its zero-based index. Requires live
 * attached instance and stable child list through synchronous notification.
 * Return zero in every case, including selection; no event-consumed flag,
 * keyboard path or resources retained.
 */
IPTR Title__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    if (!msg->imsg)
        return 0;

    switch (msg->imsg->Class)
    {
    case IDCMP_MOUSEBUTTONS:
        if (msg->imsg->Code == SELECTDOWN
            && _isinobj(msg->imsg->MouseX, msg->imsg->MouseY, obj))
        {
            struct List *children =
                (struct List *)XGET(obj, MUIA_Group_ChildList);
            APTR cstate = children->lh_Head;
            Object *child;
            int i;

            /* Find previous and next tab */
            for (i = 0; (child = NextObject(&cstate)); i++)
            {
                if (_isinobj(msg->imsg->MouseX, msg->imsg->MouseY, child))
                {
                    /* Activate this tab */
                    set(obj, MUIA_Group_ActivePage, i);
                    break;
                }
            }
        }
        break;
    }
    return 0;
}

#if ZUNE_BUILTIN_TITLE
/*
 * Route supported ABI-correct messages to local handlers, otherwise Group;
 * return selected handler result. Builtin disposal branch names
 * Title_OM_DISPOSE rather than the defined Title__OM_DISPOSE; do not infer
 * a verified build from this dispatch table. Requires live instance except
 * construction and correct lifecycle.
 */
BOOPSI_DISPATCHER(IPTR, Title_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Title__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Title__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Title_OM_DISPOSE(cl, obj, msg);
    case MUIM_Draw:
        return Title__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    case MUIM_Setup:
        return Title__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Title__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
    case MUIM_HandleEvent:
        return Title__MUIM_HandleEvent(cl, obj,
            (struct MUIP_HandleEvent *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Title_desc =
{
    MUIC_Title,
    MUIC_Group,
    sizeof(struct Title_DATA),
    (void *) Title_Dispatcher
};
#endif /* ZUNE_BUILTIN_TITLE */
