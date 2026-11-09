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

#include "debug.h"
#include "mui.h"
#include "support_classes.h"
#include "muimaster_intern.h"
#include "boopsi_private.h"

/****** Boopsi.mui/MUIC_Boopsi ************
*
*   NAME
*       MUIC_Boopsi
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Embed an Intuition BOOPSI gadget in a MUI layout. Construction creates
*       the wrapper, not the gadget, and defaults FillArea to FALSE. Show
*       creates the gadget from the selected class and a shallow clone of the
*       constructor tags, then applies remembered values and adds it to the
*       window. Hide reads remembered values, removes the gadget, disposes it
*       and clears the pointer. A later Show creates a new gadget.
*       Keep the class, ClassID string and any pointer-valued constructor data
*       alive for every Show that can use them. The wrapper owns the cloned tag
*       records and remember records, not their pointer targets. It owns each
*       created gadget. Use the normal Setup/Show/Hide/Cleanup lifecycle:
*       Cleanup only removes the event handler; Dispose frees the tag records
*       and delegates, without independently disposing a still-present gadget.
*       Repeated Show without Hide can overwrite the gadget pointer.
*       Show suppresses gadget creation if the wrapper is not completely inside
*       each enclosing virtual group. It returns the superclass result even if
*       gadget creation fails, and does not stop creation when that result is
*       zero. Partial visibility is not implemented by clipping a live gadget.
*
*   METHODS
*       Setup registers a priority-zero IDCMP_IDCMPUPDATE handler after
*       successful Area setup. Every received update is passed as tags to the
*       wrapper with private OnlyTrigger, suppressing forwarding back to the
*       gadget while still updating remembered values and sending MUI
*       notifications. Updates are not filtered by gadget ID or originating
*       object. A notification therefore does not prove that this wrapper's
*       gadget produced it.
*       Set forwards the original tag list to an existing gadget with
*       SetGadgetAttrsA, unless OnlyTrigger is present. A nonzero gadget result
*       causes RefreshGList immediately; virtual-group clipping is not added
*       here. The last NoNotify value controls temporary gadget ICA_TARGET
*       suppression. If nonzero, ICA_TARGET is set to NULL and then to
*       ICTARGET_IDCMP, not restored to its previous value. MUI notification
*       suppression is still governed independently by Notify.
*       Get searches remembered attributes first, refreshing their cached value
*       only when the live gadget getter succeeds. It then tries the gadget and
*       finally Area. The wrapper does not provide local getters for its class,
*       bounds, Smart, Remember or tag-mapping fields despite some header get
*       flags.
*       Draw delegates to Area, then refreshes a live gadget for DRAWOBJECT or
*       DRAWUPDATE and returns one. AskMinMax adds configured minima to
*       minimum/default sizes and maxima to maximum sizes after Area; it does
*       not ask the gadget for its preferred size.
*       Unhandled messages whose upper sixteen MethodID bits differ from the
*       classic MUI tag prefix are sent directly to a live gadget, with its
*       result returned. Other messages go to Area. This is a numeric-prefix
*       test, not general knowledge of all MUI extension methods.
*
*   NOTES
*       Supply GA_Left, GA_Top, GA_Width and GA_Height placeholders in the
*       constructor tags if the gadget needs layout geometry. Show replaces only
*       tags already present; it does not append missing geometry or environment
*       tags. Later ordinary Set values are not merged into the constructor
*       clone: use Remember for values that must survive Hide/Show.
*       CloneTagItems failure and individual Remember allocation failures do not
*       fail wrapper construction. There is no complete gadget-creation or
*       remembered-state error report. Use a class that creates a gadget
*       suitable for AddGadget, not an arbitrary BOOPSI object. Keep callbacks
*       and event processing serialized with Hide and disposal.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_Class ************
*
*   NAME
*       MUIA_Boopsi_Class
*
*   FUNCTION
*       Select the private IClass pointer passed to NewObjectA on Show.
*
*   INPUTS
*       Initialize and set. Borrowed struct IClass pointer; default NULL. No
*       dedicated local getter despite the header get flag.
*
*   NOTES
*       Class and ClassID are passed together to Intuition NewObjectA; a
*       non-NULL Class pointer takes precedence, otherwise ClassID selects the
*       public class. Keep the referenced class/name valid while future Show
*       calls may use it. Set updates the stored selector but does not replace a
*       current gadget; it also forwards the original tags to that gadget
*       through the ordinary setter path. The next Show uses the stored
*       selectors.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_ClassID ************
*
*   NAME
*       MUIA_Boopsi_ClassID
*
*   FUNCTION
*       Select the public class name string passed to NewObjectA on Show.
*
*   INPUTS
*       Initialize and set. Borrowed terminated string pointer; default NULL. No
*       dedicated local getter despite the header get flag.
*
*   NOTES
*       Class and ClassID are passed together to Intuition NewObjectA; a
*       non-NULL Class pointer takes precedence, otherwise ClassID selects the
*       public class. Keep the referenced class/name valid while future Show
*       calls may use it. Set updates the stored selector but does not replace a
*       current gadget; it also forwards the original tags to that gadget
*       through the ordinary setter path. The next Show uses the stored
*       selectors.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_MinWidth ************
*
*   NAME
*       MUIA_Boopsi_MinWidth
*
*   FUNCTION
*       Supply the gadget content minimum width for layout.
*
*   INPUTS
*       Initialize and set, ULONG in the header, stored as int. Default zero. No
*       dedicated local getter despite the header get flag.
*
*   NOTES
*       AskMinMax adds this value to the superclass minimum and default width.
*       No sign, overflow or minimum/maximum consistency validation is
*       performed. Set stores the value but does not explicitly request a layout
*       recalculation. The class does not query gadget dimensions to replace
*       these bounds.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_MinHeight ************
*
*   NAME
*       MUIA_Boopsi_MinHeight
*
*   FUNCTION
*       Supply the gadget content minimum height for layout.
*
*   INPUTS
*       Initialize and set, ULONG in the header, stored as int. Default zero. No
*       dedicated local getter despite the header get flag.
*
*   NOTES
*       AskMinMax adds this value to the superclass minimum and default height.
*       No sign, overflow or minimum/maximum consistency validation is
*       performed. Set stores the value but does not explicitly request a layout
*       recalculation. The class does not query gadget dimensions to replace
*       these bounds.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_MaxWidth ************
*
*   NAME
*       MUIA_Boopsi_MaxWidth
*
*   FUNCTION
*       Supply the gadget content maximum width for layout.
*
*   INPUTS
*       Initialize and set, ULONG in the header, stored as int. Default
*       MUI_MAXMAX. No dedicated local getter despite the header get flag.
*
*   NOTES
*       AskMinMax adds this value to the superclass maximum width. No sign,
*       overflow or minimum/maximum consistency validation is performed. Set
*       stores the value but does not explicitly request a layout recalculation.
*       The class does not query gadget dimensions to replace these bounds.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_MaxHeight ************
*
*   NAME
*       MUIA_Boopsi_MaxHeight
*
*   FUNCTION
*       Supply the gadget content maximum height for layout.
*
*   INPUTS
*       Initialize and set, ULONG in the header, stored as int. Default
*       MUI_MAXMAX. No dedicated local getter despite the header get flag.
*
*   NOTES
*       AskMinMax adds this value to the superclass maximum height. No sign,
*       overflow or minimum/maximum consistency validation is performed. Set
*       stores the value but does not explicitly request a layout recalculation.
*       The class does not query gadget dimensions to replace these bounds.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_Object ************
*
*   NAME
*       MUIA_Boopsi_Object
*
*   FUNCTION
*       Expose the current embedded gadget pointer through the attribute getter.
*
*   INPUTS
*       Get; borrowed Object pointer, NULL before gadget creation and after
*       Hide.
*
*   BUGS
*       The local case writes the gadget pointer but falls through to
*       remembered-attribute, gadget and superclass lookup without returning
*       success. GetAttr can therefore return FALSE after writing a valid
*       pointer, or a later successful getter can overwrite it. Do not treat
*       getter success as a reliable presence test in this implementation. A
*       captured pointer is valid only until Hide or gadget replacement; never
*       dispose it separately.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_Remember ************
*
*   NAME
*       MUIA_Boopsi_Remember
*
*   FUNCTION
*       Record a gadget attribute whose value should survive gadget recreation.
*
*   INPUTS
*       Initialize only, ULONG attribute ID. Repeat this tag for each attribute.
*       Default: no remembered attributes; no local setter/getter.
*
*   NOTES
*       Each successfully allocated record starts with the first matching
*       constructor tag value or zero. Set updates it from the first matching
*       tag. Get refreshes from the live gadget when its getter succeeds,
*       otherwise returns the cached value with TRUE. Hide calls the gadget
*       getter directly into each cached value and ignores its status. Show
*       applies the remembered tags after NewObjectA and before AddGadget.
*       Only values are stored, not deep copies of pointer targets. Do not
*       remember a pointer owned by a gadget that Hide will dispose unless the
*       underlying API guarantees a sufficient lifetime. Allocation failure
*       silently omits the new record; duplicate IDs are not deduplicated. Use
*       ordinary gadget attribute IDs, not tag-list control IDs.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_Smart ************
*
*   NAME
*       MUIA_Boopsi_Smart
*
*   FUNCTION
*       Store the constructor Smart flag without changing wrapper behavior.
*
*   INPUTS
*       Initialize only, BOOL in the header, stored as ULONG. Default zero. No
*       dedicated local setter or getter.
*
*   NOTES
*       The implementation stores this value but does not consult it in Show,
*       Hide, Draw, event routing or disposal. It does not enable another
*       clipping, rendering or lifecycle mode.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_TagScreen ************
*
*   NAME
*       MUIA_Boopsi_TagScreen
*
*   FUNCTION
*       Choose the gadget tag that receives the current screen pointer on Show.
*
*   INPUTS
*       Initialize and set, ULONG gadget attribute ID. Default zero disables
*       substitution. No dedicated local getter despite the header get flag.
*
*   NOTES
*       The corresponding gadget tag must already exist in the cloned
*       constructor list, typically with an initial zero value. Show replaces
*       its first occurrence with the borrowed current Screen pointer before
*       NewObjectA; it does not add a missing tag. Set changes the mapping for
*       future Show calls but does not rebuild the constructor list or recreate
*       a current gadget. These context pointers remain owned by the surrounding
*       screen/window lifecycle.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_TagWindow ************
*
*   NAME
*       MUIA_Boopsi_TagWindow
*
*   FUNCTION
*       Choose the gadget tag that receives the current window pointer on Show.
*
*   INPUTS
*       Initialize and set, ULONG gadget attribute ID. Default zero disables
*       substitution. No dedicated local getter despite the header get flag.
*
*   NOTES
*       The corresponding gadget tag must already exist in the cloned
*       constructor list, typically with an initial zero value. Show replaces
*       its first occurrence with the borrowed current Window pointer before
*       NewObjectA; it does not add a missing tag. Set changes the mapping for
*       future Show calls but does not rebuild the constructor list or recreate
*       a current gadget. These context pointers remain owned by the surrounding
*       screen/window lifecycle.
*
*/
/****** Boopsi.mui/MUIA_Boopsi_TagDrawInfo ************
*
*   NAME
*       MUIA_Boopsi_TagDrawInfo
*
*   FUNCTION
*       Choose the gadget tag that receives the current draw information pointer
*       on Show.
*
*   INPUTS
*       Initialize and set, ULONG gadget attribute ID. Default zero disables
*       substitution. No dedicated local getter despite the header get flag.
*
*   NOTES
*       The corresponding gadget tag must already exist in the cloned
*       constructor list, typically with an initial zero value. Show replaces
*       its first occurrence with the borrowed current DrawInfo pointer before
*       NewObjectA; it does not add a missing tag. Set changes the mapping for
*       future Show calls but does not rebuild the constructor list or recreate
*       a current gadget. These context pointers remain owned by the surrounding
*       screen/window lifecycle.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Create Area wrapper and shallow-clone constructor tags; parse
 * configuration and allocate each Remember record independently. Return
 * object or zero on superclass failure; clone/record failures do not fail
 * construction. Initialize IDCMPUPDATE handler. Pointer targets remain
 * borrowed; gadget creation is deferred to Show.
 */
IPTR Boopsi__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Boopsi_DATA *data;
    struct TagItem *tags, *tag;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_FillArea, FALSE, TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->boopsi_taglist = CloneTagItems(msg->ops_AttrList);
    data->boopsi_maxwidth = data->boopsi_maxheight = MUI_MAXMAX;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Boopsi_Class:
            data->boopsi_class = (struct IClass *)tag->ti_Data;
            break;

        case MUIA_Boopsi_ClassID:
            data->boopsi_classid = (char *)tag->ti_Data;
            break;

        case MUIA_Boopsi_MaxHeight:
            data->boopsi_maxheight = tag->ti_Data;
            break;

        case MUIA_Boopsi_MaxWidth:
            data->boopsi_maxwidth = tag->ti_Data;
            break;

        case MUIA_Boopsi_MinHeight:
            data->boopsi_minheight = tag->ti_Data;
            break;

        case MUIA_Boopsi_MinWidth:
            data->boopsi_minwidth = tag->ti_Data;
            break;

        case MUIA_Boopsi_Remember:
            {
                struct TagItem *new_remember;
                if ((new_remember = AllocVec(sizeof(struct TagItem)
                    * (data->remember_len + 2), MEMF_CLEAR)))
                        /* +2 because of the TAG_DONE */
                {
                    if (data->remember)
                        CopyMem(data->remember, new_remember,
                            sizeof(struct TagItem) * data->remember_len);
                    new_remember[data->remember_len].ti_Tag = tag->ti_Data;
                    if (data->remember)
                        FreeVec(data->remember);
                    data->remember = new_remember;
                    data->remember_len++;
                }
            }
            break;

        case MUIA_Boopsi_Smart:
            data->boopsi_smart = tag->ti_Data;
            break;

        case MUIA_Boopsi_TagDrawInfo:
            data->boopsi_tagdrawinfo = tag->ti_Data;
            break;

        case MUIA_Boopsi_TagScreen:
            data->boopsi_tagscreen = tag->ti_Data;
            break;

        case MUIA_Boopsi_TagWindow:
            data->boopsi_tagwindow = tag->ti_Data;
            break;

        }
    }

    /* Now fill in the initial remember tag datas in our remember tag list */
    for (tags = data->remember; (tag = NextTagItem(&tags));)
    {
        struct TagItem *set_tag =
            FindTagItem(tag->ti_Tag, msg->ops_AttrList);
        if (set_tag)
            tag->ti_Data = set_tag->ti_Data;
    }

    data->ehn.ehn_Events = IDCMP_IDCMPUPDATE;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    return (IPTR) obj;
}

/*
 * Free cloned constructor tags and Remember records, then delegate Area
 * disposal and return its result. Requires prior Hide/Cleanup and no
 * outstanding callbacks. Does not independently dispose a live embedded
 * gadget or release borrowed pointer targets.
 */
IPTR Boopsi__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);

    if (data->boopsi_taglist)
        FreeTagItems(data->boopsi_taglist);
    if (data->remember)
        FreeVec(data->remember);
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Store wrapper configuration and first matching Remember values, then
 * forward original tags to a live gadget unless private
 * MUIA_Boopsi_OnlyTrigger is present, regardless of its value. Last
 * NoNotify controls temporary ICA_TARGET NULL and restoration to
 * ICTARGET_IDCMP. Refresh immediately for nonzero SetGadgetAttrsA result;
 * return Area setter result. Requires valid live window context for a
 * gadget.
 */
IPTR Boopsi__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags, *tag;
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    int only_trigger = 0;
    int no_notify = 0;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Boopsi_Class:
            data->boopsi_class = (struct IClass *)tag->ti_Data;
            break;

        case MUIA_Boopsi_ClassID:
            data->boopsi_classid = (char *)tag->ti_Data;
            break;

        case MUIA_Boopsi_MaxHeight:
            data->boopsi_maxheight = tag->ti_Data;
            break;

        case MUIA_Boopsi_MaxWidth:
            data->boopsi_maxwidth = tag->ti_Data;
            break;

        case MUIA_Boopsi_MinHeight:
            data->boopsi_minheight = tag->ti_Data;
            break;

        case MUIA_Boopsi_MinWidth:
            data->boopsi_minwidth = tag->ti_Data;
            break;

        case MUIA_Boopsi_TagDrawInfo:
            data->boopsi_tagdrawinfo = tag->ti_Data;
            break;

        case MUIA_Boopsi_TagScreen:
            data->boopsi_tagscreen = tag->ti_Data;
            break;

        case MUIA_Boopsi_TagWindow:
            data->boopsi_tagwindow = tag->ti_Data;
            break;

        case MUIA_Boopsi_OnlyTrigger:
            only_trigger = 1;
            break;

        case MUIA_NoNotify:
            no_notify = tag->ti_Data;
            break;
        }
    }

    /* Now fill in remember list tag datas in our remember tag list */
    for (tags = data->remember; (tag = NextTagItem(&tags));)
    {
        struct TagItem *set_tag =
            FindTagItem(tag->ti_Tag, msg->ops_AttrList);
        if (set_tag)
            tag->ti_Data = set_tag->ti_Data;
    }

    if (!only_trigger)
    {
        if (data->boopsi_object)
        {
            /* Rendering will happen here!! This could make problems with
             * virtual groups, forward this to MUIM_Draw??? */
            if (no_notify)
                SetAttrs(data->boopsi_object, ICA_TARGET, NULL, TAG_DONE);
            if (SetGadgetAttrsA((struct Gadget *)data->boopsi_object,
                    _window(obj), NULL, msg->ops_AttrList))
                RefreshGList((struct Gadget *)data->boopsi_object,
                    _window(obj), NULL, 1);
            if (no_notify)
                SetAttrs(data->boopsi_object, ICA_TARGET, ICTARGET_IDCMP,
                    TAG_DONE);
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


#define STORE *(msg->opg_Storage)
/*
 * Search remembered values, refreshing only on successful gadget GetAttr;
 * otherwise try gadget then Area and return selected status. Requires
 * writable storage and live pointer targets. Object writes gadget pointer
 * but falls through, so later lookup may overwrite storage or return
 * failure. No dedicated configuration getters.
 */
IPTR Boopsi__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Boopsi_Object:
        STORE = (IPTR) data->boopsi_object;

    default:
        {
            struct TagItem *tags, *tag;

            /* look in the rember list first */
            for (tags = data->remember; (tag = NextTagItem(&tags));)
            {
                if (tag->ti_Tag == msg->opg_AttrID)
                {
                    if (data->boopsi_object)
                    {
                        /* Call the get method of the boopsi object
                         * and update the remember list */
                        IPTR val;

                        if (GetAttr(msg->opg_AttrID, data->boopsi_object,
                                &val))
                            tag->ti_Data = val;
                    }

                    STORE = tag->ti_Data;
                    return 1;
                }
            }

            /* The id is not in the attr list, so we try the boopsi object
             * first (fills in the msg then) */
            if (data->boopsi_object)
            {
                IPTR val;

                if (GetAttr(msg->opg_AttrID, data->boopsi_object, &val))
                {
                    STORE = val;
                    return 1;
                }
            }

            /* No success so we try the superclass */
            return DoSuperMethodA(cl, obj, (Msg) msg);
        }
    }

    return 1;
}
#undef STORE

/*
 * Delegate Area sizing, add stored minima to minimum/default and maxima to
 * maximum fields, and return TRUE. Requires writable MinMaxInfo and
 * sensible int bounds; no validation or gadget size query. No object
 * ownership transfer.
 */
IPTR Boopsi__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);

    /*
     ** let our superclass first fill in what it thinks about sizes.
     ** this will e.g. add the size of frame and inner spacing.
     */
    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += data->boopsi_minwidth;
    msg->MinMaxInfo->MinHeight += data->boopsi_minheight;
    msg->MinMaxInfo->DefWidth += data->boopsi_minwidth;
    msg->MinMaxInfo->DefHeight += data->boopsi_minheight;
    msg->MinMaxInfo->MaxWidth += data->boopsi_maxwidth;
    msg->MinMaxInfo->MaxHeight += data->boopsi_maxheight;
    return TRUE;
}

/*
 * Delegate Area Setup; return zero on failure. Register IDCMPUPDATE handler
 * on live parent Window and return one without checking registration
 * result. Requires connected object; pair with Cleanup.
 */
IPTR Boopsi__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    IPTR rc = DoSuperMethodA(cl, obj, (Msg) msg);
    if (!rc)
        return 0;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    return 1;
}

/*
 * Remove handler from live parent Window and return Area cleanup result.
 * Requires paired Setup and prior Hide for an embedded gadget; does not
 * dispose the gadget or free constructor/Remember records.
 */
IPTR Boopsi__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Delegate Area Show, test complete virtual-group visibility, replace only
 * existing geometry/context tags, create gadget, apply Remember tags and
 * AddGadget. Return superclass result regardless of creation outcome; zero
 * superclass result does not stop work. Requires valid class/borrowed tag
 * targets and window context, plus no prior live gadget.
 */
IPTR Boopsi__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    IPTR rc = DoSuperMethodA(cl, obj, (Msg) msg);
    struct TagItem *tag;
    BOOL completely_visible = TRUE;

    D(bug("boopsi_show: obj coord %d,%d - %d,%d\n",
            _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj)));

    if (_flags(obj) & MADF_INVIRTUALGROUP)
    {
        Object *wnd = NULL, *parent;

        get(obj, MUIA_WindowObject, &wnd);
        parent = obj;
        while (get(parent, MUIA_Parent, &parent))
        {
            if (!parent)
                break;
            if (parent == wnd)
                break;

            if (_flags(parent) & MADF_ISVIRTUALGROUP)
            {
                if ((_mleft(obj) < _mleft(parent)) ||
                    (_mright(obj) > _mright(parent)) ||
                    (_mtop(obj) < _mtop(parent)) ||
                    (_mbottom(obj) > _mbottom(parent)))
                {
                    completely_visible = FALSE;
                    D(bug("=== boopsi object: completely visible FALSE "
                        "for obj %x at %d,%d - %d,%d\n",
                        obj, _mleft(obj), _mtop(obj), _mright(obj),
                        _mbottom(obj)));
                    break;
                }
            }
        }
    }

    if (completely_visible)
    {
        if ((tag = FindTagItem(GA_Left, data->boopsi_taglist)))
            tag->ti_Data = _mleft(obj);
        if ((tag = FindTagItem(GA_Top, data->boopsi_taglist)))
            tag->ti_Data = _mtop(obj);
        if ((tag = FindTagItem(GA_Width, data->boopsi_taglist)))
            tag->ti_Data = _mwidth(obj);
        if ((tag = FindTagItem(GA_Height, data->boopsi_taglist)))
            tag->ti_Data = _mheight(obj);
        if (data->boopsi_tagscreen
            && (tag =
                FindTagItem(data->boopsi_tagscreen, data->boopsi_taglist)))
            tag->ti_Data = (IPTR) _screen(obj);
        if (data->boopsi_tagwindow
            && (tag =
                FindTagItem(data->boopsi_tagwindow, data->boopsi_taglist)))
            tag->ti_Data = (IPTR) _window(obj);
        if (data->boopsi_tagdrawinfo
            && (tag =
                FindTagItem(data->boopsi_tagdrawinfo,
                    data->boopsi_taglist)))
            tag->ti_Data = (IPTR) _dri(obj);

        if ((data->boopsi_object =
                NewObjectA(data->boopsi_class, data->boopsi_classid,
                    data->boopsi_taglist)))
        {
            SetAttrsA(data->boopsi_object, data->remember);
            AddGadget(_window(obj), (struct Gadget *)data->boopsi_object, ~0);
        }
    }

    return rc;
}

/*
 * Delegate Area Draw, then RefreshGList for live gadget on object/update
 * flags and return one. Requires valid window and render lifecycle. No
 * clipping added for virtual groups, no error or ownership result.
 */
IPTR Boopsi__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    DoSuperMethodA(cl, obj, (Msg) msg);

    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE)))
        return 1;
    if (data->boopsi_object)
        RefreshGList((struct Gadget *)data->boopsi_object, _window(obj),
            NULL, 1);
    return 1;
}

/*
 * Read remembered gadget attributes directly into cache without checking
 * status, remove/dispose live gadget and clear pointer, then return Area
 * Hide result. Requires live parent Window and no concurrent gadget
 * callbacks; cached pointers are not deep-copied and may dangle after
 * disposal.
 */
IPTR Boopsi__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct Boopsi_DATA *data = INST_DATA(cl, obj);
    if (data->boopsi_object)
    {
        struct TagItem *tags, *tag;

        /* Fill in the initial remember tag data in our remember tag list */
        for (tags = data->remember; (tag = NextTagItem(&tags));)
        {
            GetAttr(tag->ti_Tag, data->boopsi_object, &tag->ti_Data);
        }

        RemoveGadget(_window(obj), (struct Gadget *)data->boopsi_object);
        DisposeObject(data->boopsi_object);
        data->boopsi_object = NULL;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * For a non-NULL IDCMPUPDATE message, pass its readable IAddress tag list
 * to Set with private OnlyTrigger. Requires live wrapper for synchronous
 * notifications; does not check originating gadget ID or consume/reply to
 * the message. Return zero, including unrelated events.
 */
IPTR Boopsi__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    //struct Boopsi_DATA *data = INST_DATA(cl, obj);

    if (msg->imsg)
    {
        if (msg->imsg->Class == IDCMP_IDCMPUPDATE)
        {
//          struct TagItem *tags,*tag;

            /* Go through the tag list (iaddress) and set all the tags;
             ** this is somewhat stupid, because all objects which hear on
             ** the same notifies will receive the notify although their
             ** value has not changed really, but this is how MUI seems to
             ** mange it (this could really make big problems if using
             ** MUIV_TriggerValue). A better idea, is to distinguish this
             ** via an allocatable gadgetid, and the object checks wheather
             ** the gadget id equals to its allocated one
             **/

            SetAttrs(obj, MUIA_Boopsi_OnlyTrigger, TRUE, TAG_MORE,
                (IPTR) msg->imsg->IAddress);

//            for (tags = (struct TagItem*)msg->imsg->IAddress;
//                (tag = NextTagItem(&tags)); )
//            {
//            }
        }
    }

    return 0;
}

#if ZUNE_BUILTIN_BOOPSI
/*
 * Dispatch local overrides; for other methods send to live gadget if upper
 * sixteen MethodID bits differ from classic MUI prefix, otherwise to Area.
 * Return receiver result. Requires valid ABI/message and live object except
 * construction; prefix test is not a general MUI-extension classification.
 */
BOOPSI_DISPATCHER(IPTR, Boopsi_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Boopsi__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Boopsi__OM_DISPOSE(cl, obj, msg);
    case OM_GET:
        return Boopsi__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_SET:
        return Boopsi__OM_SET(cl, obj, (struct opSet *)msg);
    case MUIM_Setup:
        return Boopsi__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Boopsi__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_Show:
        return Boopsi__MUIM_Show(cl, obj, (APTR) msg);
    case MUIM_Hide:
        return Boopsi__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_AskMinMax:
        return Boopsi__MUIM_AskMinMax(cl, obj, (APTR) msg);
    case MUIM_Draw:
        return Boopsi__MUIM_Draw(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Boopsi__MUIM_HandleEvent(cl, obj, (APTR) msg);
    }
    {
        struct Boopsi_DATA *data = INST_DATA(cl, obj);
        if (((msg->MethodID >> 16) != ((TAG_USER >> 16) | 0x0042))
            && data->boopsi_object)
        {
            return DoMethodA(data->boopsi_object, msg);
        }
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Boopsi_desc =
{
    MUIC_Boopsi,
    MUIC_Area,
    sizeof(struct Boopsi_DATA),
    (void *) Boopsi_Dispatcher
};
#endif /* ZUNE_BUILTIN_BOOPSI */
