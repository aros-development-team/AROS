/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "scrollgroup_private.h"

/****** Scrollgroup.mui/MUIC_Scrollgroup ************
*
*   NAME
*       MUIC_Scrollgroup
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Wrap virtual contents with horizontal and vertical Scrollbar objects and
*       a corner Scrollbutton. Supply a live, unowned Virtgroup-compatible
*       Contents object. The constructor creates an internal Group with a
*       private layout hook and adopts contents and controls into it; the outer
*       Scrollgroup owns that Group. Returned Contents/HorizBar/VertBar pointers
*       are borrowed.
*       The layout hook is allocated separately and owned by Scrollgroup.
*       Dispose frees it before Group disposes children. End active Show/Setup
*       resources before disposal. Construction returns NULL on hook or
*       superclass construction failure. Nested constructor failures do not
*       provide a comprehensive ownership-unwind guarantee; do not assume every
*       unattached temporary is freed.
*
*   METHODS
*       The private layout hook computes constraints and allocates viewport
*       space, showing only the required in-content bars. A corner button is
*       shown when both bars are needed. No notification wiring from that button
*       to the scroll offsets is installed here, so its presence does not
*       establish working two-axis drag scrolling.
*       Show reads content Left/Top/Width/Height, writes each bar's
*       Prop_First/Entries/Visible from offsets, content extents and inner
*       viewport dimensions, then returns Group Show's result. Getter/setter
*       results during synchronization are ignored. Range synchronization is
*       performed by this Show override, not by the private layout hook itself.
*       Notifications connect bar Prop_First changes to content offsets, and
*       content offset changes back to the bars. The callback uses NoNotify on
*       destination updates to prevent feedback. Bar-to-content writes also
*       disable Group child forwarding. Registration results are ignored.
*
*   NOTES
*       UseWinBorder selects existing bottom/right window proportional gadgets
*       instead of laying bars beside the content. Those gadgets must be
*       supplied by the enclosing window; this class does not request window
*       border scrollers itself.
*       AutoBars, FreeHoriz, FreeVert, NoHorizBar and NoVertBar are declared but
*       not implemented. Adding unrelated children or replacing the internal
*       layout hook does not extend the fixed-child layout algorithm. Retain the
*       constructed tree and internal callback targets throughout its lifetime.
*
*   BUGS
*       When adding a vertical bar, the second horizontal extent clamp uses
*       virt_maxheight rather than virt_maxwidth. Bar selection can therefore be
*       wrong for differing width/height limits. Min/max calculations reserve
*       both bar minima even when a bar will be hidden. Arithmetic includes
*       unchecked WORD intermediates. The layout hook ignores child layout
*       results; Group ignores the hook return value.
*       Content reads use the get macro with LONG destinations. Its GNU C
*       implementation uses a temporary union; the non-GNU fallback casts the
*       destination to an IPTR pointer. Do not assume identical width handling
*       on all compiler/ABI combinations. Contents must implement the expected
*       virtual getters and lifecycle; unsupported content types are not
*       validated.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_Contents ************
*
*   NAME
*       MUIA_Scrollgroup_Contents
*
*   FUNCTION
*       Supply the virtual content tree to be adopted by the internal Group.
*
*   INPUTS
*       Object pointer; initialize and get. Required non-NULL, unowned
*       Virtgroup-compatible object.
*
*   NOTES
*       The getter returns the same borrowed object. Do not dispose or detach it
*       independently while Scrollgroup uses it. There is no local setter or
*       replacement mechanism. A NULL child makes nested Group construction
*       fail, but nested allocations and ownership on failure are not
*       comprehensively unwound. Use actual virtual Group semantics, including
*       private content-minimum getters.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_HorizBar ************
*
*   NAME
*       MUIA_Scrollgroup_HorizBar
*
*   FUNCTION
*       Return the internally created horizontal Scrollbar object.
*
*   INPUTS
*       Object pointer; get only.
*
*   NOTES
*       The returned object is borrowed and owned through the internal Group.
*       Use its Prop attributes for range/position; synchronization
*       notifications are already installed. Do not dispose or remove it
*       independently. This is a Scrollbar object, not the raw Intuition
*       proportional gadget. No local setter exists.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_VertBar ************
*
*   NAME
*       MUIA_Scrollgroup_VertBar
*
*   FUNCTION
*       Return the internally created vertical Scrollbar object.
*
*   INPUTS
*       Object pointer; get only.
*
*   NOTES
*       The returned object is borrowed and owned through the internal Group.
*       Use its Prop attributes for range/position; synchronization
*       notifications are already installed. Do not dispose or remove it
*       independently. This is a Scrollbar object, not the raw Intuition
*       proportional gadget. No local setter exists.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_UseWinBorder ************
*
*   NAME
*       MUIA_Scrollgroup_UseWinBorder
*
*   FUNCTION
*       Choose bottom and right window-border scrolling when nonzero.
*
*   INPUTS
*       BOOL; initialize only. Default FALSE.
*
*   NOTES
*       The constructor passes Prop_UseWinBorder_Bottom/Right to its bars. In
*       this mode the layout uses the full available content rectangle and does
*       not perform automatic in-content bar placement. Configure the enclosing
*       window with Window_UseBottomBorderScroller and
*       Window_UseRightBorderScroller to provide the matching gadgets. There is
*       no local getter or runtime switch; the corner button is still
*       constructed but is not explicitly laid out by this branch.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_AutoBars ************
*
*   NAME
*       MUIA_Scrollgroup_AutoBars
*
*   FUNCTION
*       Declared attribute without a local implementation in AROS Scrollgroup.
*
*   INPUTS
*       BOOL; header declares initialize, set and get.
*
*   NOTES
*       No constructor, setter, getter or layout branch consumes this attribute.
*       In-content bar visibility is automatically selected by the private
*       layout hook regardless of this value. Group fallback to child getters
*       does not establish local support. No working default or state should be
*       inferred from the declaration.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_FreeHoriz ************
*
*   NAME
*       MUIA_Scrollgroup_FreeHoriz
*
*   FUNCTION
*       Declared attribute without a local implementation in AROS Scrollgroup.
*
*   INPUTS
*       BOOL; header declares initialize only.
*
*   NOTES
*       No constructor, setter, getter or layout branch consumes this attribute.
*       This value does not constrain the content axis; layout uses actual
*       content min/max information. Group fallback to child getters does not
*       establish local support. No working default or state should be inferred
*       from the declaration.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_FreeVert ************
*
*   NAME
*       MUIA_Scrollgroup_FreeVert
*
*   FUNCTION
*       Declared attribute without a local implementation in AROS Scrollgroup.
*
*   INPUTS
*       BOOL; header declares initialize only.
*
*   NOTES
*       No constructor, setter, getter or layout branch consumes this attribute.
*       This value does not constrain the content axis; layout uses actual
*       content min/max information. Group fallback to child getters does not
*       establish local support. No working default or state should be inferred
*       from the declaration.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_NoHorizBar ************
*
*   NAME
*       MUIA_Scrollgroup_NoHorizBar
*
*   FUNCTION
*       Declared attribute without a local implementation in AROS Scrollgroup.
*
*   INPUTS
*       BOOL; header declares initialize, set and get.
*
*   NOTES
*       No constructor, setter, getter or layout branch consumes this attribute.
*       In-content bar visibility is automatically selected by the private
*       layout hook regardless of this value. Group fallback to child getters
*       does not establish local support. No working default or state should be
*       inferred from the declaration.
*
*/
/****** Scrollgroup.mui/MUIA_Scrollgroup_NoVertBar ************
*
*   NAME
*       MUIA_Scrollgroup_NoVertBar
*
*   FUNCTION
*       Declared attribute without a local implementation in AROS Scrollgroup.
*
*   INPUTS
*       BOOL; header declares initialize, set and get.
*
*   NOTES
*       No constructor, setter, getter or layout branch consumes this attribute.
*       In-content bar visibility is automatically selected by the private
*       layout hook regardless of this value. Group fallback to child getters
*       does not establish local support. No working default or state should be
*       inferred from the declaration.
*
*/

extern struct Library *MUIMasterBase;


/*
 * Private Group layout hook; h_Data must be live Scrollgroup_DATA with four
 * valid child references. MINMAX fills six fields, returning zero. LAYOUT
 * places fixed children and sets visibility, returning one; unknown types
 * return zero. Requires prior child min/max and valid virtual getters.
 * Ignores MUI_Layout results; Group ignores hook result. The vertical-bar
 * branch incorrectly clamps width with maximum height. Content reads use
 * get with LONG destinations; GNU C uses a temporary union, while the
 * fallback casts to IPTR storage.
 */
AROS_UFH3(ULONG, Scrollgroup_Layout_Function,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct MUI_LayoutMsg *, lm, A1))
{
    AROS_USERFUNC_INIT

    struct Scrollgroup_DATA *data =
        (struct Scrollgroup_DATA *)hook->h_Data;

    switch (lm->lm_Type)
    {
    case MUILM_MINMAX:
        {
            /* Calculate the minmax dimension of the group.
             ** We only have a fixed number of children, so we need
             ** no NextObject()
             */

            if (data->usewinborder)
            {
                lm->lm_MinMax.MinWidth = _minwidth(data->contents);
                lm->lm_MinMax.MinHeight = _minheight(data->contents);
                lm->lm_MinMax.DefWidth = _defwidth(data->contents);
                lm->lm_MinMax.DefHeight = _defheight(data->contents);
                lm->lm_MinMax.MaxWidth = _maxwidth(data->contents);
                lm->lm_MinMax.MaxHeight = _maxheight(data->contents);

            }
            else
            {
                WORD maxxxxwidth = 0;
                WORD maxxxxheight = 0;

                maxxxxwidth = _minwidth(data->contents);
                if (_minwidth(data->horiz) > maxxxxwidth)
                    maxxxxwidth = _minwidth(data->horiz);
                maxxxxwidth += _minwidth(data->vert);
                lm->lm_MinMax.MinWidth = maxxxxwidth;

                maxxxxheight = _minheight(data->contents);
                if (_minheight(data->vert) > maxxxxheight)
                    maxxxxheight = _minheight(data->vert);
                maxxxxheight += _minheight(data->horiz);
                lm->lm_MinMax.MinHeight = maxxxxheight;

                maxxxxwidth = _defwidth(data->contents);
                if (_defwidth(data->horiz) > maxxxxwidth)
                    maxxxxwidth = _defwidth(data->horiz);
                if (maxxxxwidth < lm->lm_MinMax.MinWidth)
                    maxxxxwidth = lm->lm_MinMax.MinWidth;
                lm->lm_MinMax.DefWidth = maxxxxwidth;

                maxxxxheight = _defheight(data->contents);
                if (_defheight(data->vert) > maxxxxheight)
                    maxxxxheight = _defheight(data->vert);
                if (maxxxxheight < lm->lm_MinMax.MinHeight)
                    maxxxxheight = lm->lm_MinMax.MinHeight;
                lm->lm_MinMax.DefHeight = maxxxxheight;

                lm->lm_MinMax.MaxWidth = MUI_MAXMAX;
                lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
            }

            //kprintf("scrollgroup minmax: min %d x %d  def %d x %d\n",
            //        lm->lm_MinMax.MinWidth, lm->lm_MinMax.MinHeight,
            //        lm->lm_MinMax.DefWidth, lm->lm_MinMax.DefHeight);

            //kprintf("contents minmax: min %d x %d  def %d x %d\n",
            //        _minwidth(data->contents), _minheight(data->contents),
            //        _defwidth(data->contents), _defheight(data->contents));

            return 0;
        }

    case MUILM_LAYOUT:
        {
            /* Now place the objects between
             * (0,0,lm->lm_Layout.Width-1,lm->lm_Layout.Height-1)
             */

            LONG virt_width;
            LONG virt_height;
            LONG virt_minwidth = 0;
            LONG virt_minheight = 0;
            LONG virt_maxwidth;
            LONG virt_maxheight;
            LONG vert_width = _minwidth(data->vert);
            LONG horiz_height = _minheight(data->horiz);
            LONG lay_width = lm->lm_Layout.Width;
            LONG lay_height = lm->lm_Layout.Height;
            LONG cont_width = lay_width - _subwidth(data->contents);
            LONG cont_height = lay_height - _subheight(data->contents);
            BOOL vbar = FALSE, hbar = FALSE;

            if (!data->usewinborder)
            {
                //kprintf("scrollgroup layout: "
                //    "%d x %d  sub contents size %d,%d\n",
                //    lay_width, lay_height, _subwidth(data->contents),
                //    _subheight(data->contents));

                /* layout the virtual group a first time, to determine
                 * the virtual width/height */
                //MUI_Layout(data->contents,0,0,lay_width,lay_height,0);

                get(data->contents, MUIA_Virtgroup_MinWidth,
                    &virt_minwidth);
                get(data->contents, MUIA_Virtgroup_MinHeight,
                    &virt_minheight);
                virt_minwidth -= _subwidth(data->contents);
                virt_minheight -= _subheight(data->contents);

                virt_maxwidth =
                    _maxwidth(data->contents) - _subwidth(data->contents);
                virt_maxheight =
                    _maxheight(data->contents) - _subheight(data->contents);

                virt_width =
                    CLAMP(cont_width, virt_minwidth, virt_maxwidth);
                virt_height =
                    CLAMP(cont_height, virt_minheight, virt_maxheight);

                if (virt_width > cont_width)
                {
                    cont_height -= horiz_height;
                    virt_height =
                        CLAMP(cont_height, virt_minheight, virt_maxheight);
                    hbar = TRUE;
                }
                //kprintf("      1: %d x %d   hbar %d  vbar %d\n",
                //    cont_width, cont_height, hbar, vbar);

                if (virt_height > cont_height)
                {
                    cont_width -= vert_width;
                    virt_width =
                        CLAMP(cont_width, virt_minwidth, virt_maxheight);
                    vbar = TRUE;
                }
                //kprintf("      2: %d x %d   hbar %d  vbar %d\n",
                //    cont_width, cont_height, hbar, vbar);

                /* We need to check this a 2nd time!! */
                if (!hbar && (virt_width > cont_width))
                {
                    cont_height -= horiz_height;
                    virt_height =
                        CLAMP(cont_height, virt_minheight, virt_maxheight);
                    hbar = TRUE;
                }
                //kprintf("      3: %d x %d   hbar %d  vbar %d\n",
                //    cont_width, cont_height, hbar, vbar);
            }

            cont_width += _subwidth(data->contents);
            cont_height += _subheight(data->contents);

            //kprintf("cont_size layouted to %d,%d\n",
            //    cont_width, cont_height);

            if (hbar && vbar)
            {
                /* We need all scrollbars and the button */
                set(data->vert, MUIA_ShowMe, TRUE);
                    /* We could also overload MUIM_Show... */
                set(data->horiz, MUIA_ShowMe, TRUE);
                set(data->button, MUIA_ShowMe, TRUE);
                MUI_Layout(data->vert, cont_width, 0, vert_width,
                    cont_height, 0);
                MUI_Layout(data->horiz, 0, cont_height, cont_width,
                    horiz_height, 0);
                MUI_Layout(data->button, cont_width, cont_height,
                    vert_width, horiz_height, 0);
            }
            else if (vbar)
            {
                set(data->vert, MUIA_ShowMe, TRUE);
                set(data->horiz, MUIA_ShowMe, FALSE);
                set(data->button, MUIA_ShowMe, FALSE);

                MUI_Layout(data->vert, cont_width, 0, vert_width,
                    cont_height, 0);
            }
            else if (hbar)
            {
                set(data->vert, MUIA_ShowMe, FALSE);
                set(data->horiz, MUIA_ShowMe, TRUE);
                set(data->button, MUIA_ShowMe, FALSE);

                MUI_Layout(data->horiz, 0, cont_height, cont_width,
                    horiz_height, 0);
            }
            else if (!data->usewinborder)
            {
                set(data->vert, MUIA_ShowMe, FALSE);
                set(data->horiz, MUIA_ShowMe, FALSE);
                set(data->button, MUIA_ShowMe, FALSE);
            }

            /* Layout the group a second time, note that setting _mwidth()
             * and _mheight() should be enough, or we invent a new flag */
            MUI_Layout(data->contents, 0, 0, cont_width, cont_height, 0);

            //kprintf(" contents size after layout: %d x %d inner %d x %d\n",
            //        _width(data->contents), _height(data->contents),
            //        _mwidth(data->contents), _mheight(data->contents));
            return 1;
        }
    }
    return 0;

    AROS_USERFUNC_EXIT
}


/*
 * Private notification callback. Requires h_Data and a message containing
 * two pointer-sized slots, type and value. Types 1/2 read
 * vertical/horizontal Prop_First into SIPTR and update content Top/Left
 * with NoNotify and Forward FALSE; types 3/4 update horizontal/vertical
 * First with NoNotify. Ignore getter/setter results, return zero even for
 * unknown type. Borrowed message/objects; no lifetime protection against
 * disposal in callbacks.
 */
AROS_UFH3(ULONG, Scrollgroup_Function,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, dummy, A2),
    AROS_UFHA(void **, msg, A1))
{
    AROS_USERFUNC_INIT

    struct Scrollgroup_DATA *data =
        (struct Scrollgroup_DATA *)hook->h_Data;

    SIPTR type = (SIPTR) msg[0];
    SIPTR val = (SIPTR) msg[1];

    switch (type)
    {
    case 1:
        {
            get(data->vert, MUIA_Prop_First, &val);
            SetAttrs(data->contents, MUIA_Virtgroup_Top, val, MUIA_NoNotify,
                TRUE, MUIA_Group_Forward, FALSE, TAG_DONE);
            break;
        }

    case 2:
        {
            get(data->horiz, MUIA_Prop_First, &val);
            SetAttrs(data->contents, MUIA_Virtgroup_Left, val,
                MUIA_NoNotify, TRUE, MUIA_Group_Forward, FALSE, TAG_DONE);
            break;
        }
    case 3:
        nnset(data->horiz, MUIA_Prop_First, val);
        break;
    case 4:
        nnset(data->vert, MUIA_Prop_First, val);
        break;
    }
    return 0;

    AROS_USERFUNC_EXIT
}

/*
 * Allocate an owned layout Hook and construct a nested Group with adopted
 * virtual contents, two Scrollbars and corner Scrollbutton. Initialize
 * borrowed internal pointers and hook data only after superclass success;
 * install bidirectional offset notifications ignoring results. Return
 * object or zero, freeing the hook on superclass failure. Nested object
 * allocation failures do not have comprehensive local unwind; requires
 * unowned compatible contents and writable tags.
 */
IPTR Scrollgroup__OM_NEW(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct Scrollgroup_DATA *data;
    //struct TagItem *tags,*tag;
    Object *contents =
        (Object *) GetTagData(MUIA_Scrollgroup_Contents, 0,
        msg->ops_AttrList);
    Object *vert, *horiz, *button, *group;
    BOOL usewinborder;
    Tag hbordertag = TAG_IGNORE;
    Tag vbordertag = TAG_IGNORE;

    struct Hook *layout_hook = mui_alloc_struct(struct Hook);
    if (!layout_hook)
        return 0;

    layout_hook->h_Entry = (HOOKFUNC) Scrollgroup_Layout_Function;

    usewinborder =
        GetTagData(MUIA_Scrollgroup_UseWinBorder, FALSE, msg->ops_AttrList);
    if (usewinborder)
    {
        hbordertag = MUIA_Prop_UseWinBorder;
        vbordertag = MUIA_Prop_UseWinBorder;
    }

    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL,
        MUIA_Group_Horiz, FALSE,
        Child, (IPTR) (group = (Object *) GroupObject,
            MUIA_Group_LayoutHook, (IPTR) layout_hook,
            Child, (IPTR) contents,
            Child, (IPTR) (vert =
                (Object *) ScrollbarObject, MUIA_Group_Horiz, FALSE,
                vbordertag, MUIV_Prop_UseWinBorder_Right,
                End),
            Child, (IPTR) (horiz =
                (Object *) ScrollbarObject, MUIA_Group_Horiz, TRUE,
                hbordertag, MUIV_Prop_UseWinBorder_Bottom,
                End),
            Child, (IPTR) (button =
                (Object *) ScrollbuttonObject,
                End),
            End),
        TAG_MORE, msg->ops_AttrList);

    if (!obj)
    {
        mui_free(layout_hook);
        return 0;
    }

    data = INST_DATA(cl, obj);
    data->vert = vert;
    data->horiz = horiz;
    data->button = button;
    data->contents = contents;
    data->usewinborder = usewinborder;

    data->hook.h_Entry = (HOOKFUNC) Scrollgroup_Function;
    data->hook.h_Data = data;
    data->layout_hook = layout_hook;
    layout_hook->h_Data = data;

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR) obj,
        4, MUIM_CallHook, (IPTR) & data->hook, 1, MUIV_TriggerValue);
    DoMethod(horiz, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) & data->hook, 2,
        MUIV_TriggerValue);
    DoMethod(contents, MUIM_Group_DoMethodNoForward, MUIM_Notify,
        MUIA_Virtgroup_Left, MUIV_EveryTime, (IPTR) obj, 4, MUIM_CallHook,
        (IPTR) & data->hook, 3, MUIV_TriggerValue);
    DoMethod(contents, MUIM_Group_DoMethodNoForward, MUIM_Notify,
        MUIA_Virtgroup_Top, MUIV_EveryTime, (IPTR) obj, 4, MUIM_CallHook,
        (IPTR) & data->hook, 4, MUIV_TriggerValue);

    D(bug("Scrollgroup_New(0x%p)\n", obj));
    D(bug(" vert = 0x%p, horiz = 0x%p, button = 0x%p\n", vert, horiz, button));
    return (IPTR) obj;
}

/*
 * Return borrowed Contents, HorizBar or VertBar through caller IPTR storage
 * with result one. Delegate all other getters to Group. Objects remain
 * owned by the constructed child tree; caller must not dispose or detach
 * them independently.
 */
IPTR Scrollgroup__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Scrollgroup_DATA *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch (msg->opg_AttrID)
    {
    case MUIA_Scrollgroup_Contents:
        STORE = (IPTR) data->contents;
        return 1;
    case MUIA_Scrollgroup_HorizBar:
        STORE = (IPTR) data->horiz;
        return 1;
    case MUIA_Scrollgroup_VertBar:
        STORE = (IPTR) data->vert;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Free the owned layout Hook then dispose Group and its child tree,
 * returning superclass result. Requires ended display lifecycle and no
 * active callbacks retaining the hook or instance; child pointers are not
 * separately freed here.
 */
IPTR Scrollgroup__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Scrollgroup_DATA *data = INST_DATA(cl, obj);
    mui_free(data->layout_hook);
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Read virtual offsets/extents and set bar First/Entries/Visible before
 * calling Group Show. Return superclass result, ignoring all
 * synchronization statuses. Requires completed content layout and valid
 * children. The get macro mediates LONG destinations differently for GNU C
 * and its fallback; initialization to zero is not getter validation.
 */
IPTR Scrollgroup__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct Scrollgroup_DATA *data = INST_DATA(cl, obj);
    LONG top = 0, left = 0, width = 0, height = 0;

    get(data->contents, MUIA_Virtgroup_Left, &left);
    get(data->contents, MUIA_Virtgroup_Top, &top);
    get(data->contents, MUIA_Virtgroup_Width, &width);
    get(data->contents, MUIA_Virtgroup_Height, &height);

    SetAttrs(data->horiz, MUIA_Prop_First, left,
        MUIA_Prop_Entries, width,
        MUIA_Prop_Visible, _mwidth(data->contents), TAG_DONE);

    SetAttrs(data->vert, MUIA_Prop_First, top,
        MUIA_Prop_Entries, height,
        MUIA_Prop_Visible, _mheight(data->contents), TAG_DONE);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#if ZUNE_BUILTIN_SCROLLGROUP
/*
 * When built in, route New/Get/Dispose/Show locally and delegate all other
 * messages to Group. Return selected handler result. Requires valid
 * object/class and ABI-correct message; no local setter implements the
 * unused declared attributes.
 */
BOOPSI_DISPATCHER(IPTR, Scrollgroup_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Scrollgroup__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Scrollgroup__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_DISPOSE:
        return Scrollgroup__OM_DISPOSE(cl, obj, msg);
    case MUIM_Show:
        return Scrollgroup__MUIM_Show(cl, obj, (struct MUIP_Show *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Scrollgroup_desc =
{
    MUIC_Scrollgroup,
    MUIC_Group,
    sizeof(struct Scrollgroup_DATA),
    (void *) Scrollgroup_Dispatcher
};
#endif /* ZUNE_BUILTIN_SCROLLGROUP */
