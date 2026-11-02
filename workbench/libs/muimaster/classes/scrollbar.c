/*
    Copyright (C)  2002-2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include "../datatypescache.h"
#include "../imspec_intern.h"

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "prefs.h"

#define MYDEBUG 1
#include "debug.h"

/****** Scrollbar.mui/MUIC_Scrollbar ************
*
*   NAME
*       MUIC_Scrollbar
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Combine a Prop child with decrease/increase arrow Images. Group_Horiz
*       chooses orientation at construction; default FALSE is vertical. Without
*       Prop_UseWinBorder, arrows use Timer notifications to call
*       Prop_Decrease/Increase. With a nonzero border selector, only the Prop is
*       attached and the Scrollbar receives the border-gadget flag.
*       Attached Prop/arrows are Group-owned. Scrollbar has no local disposer;
*       normal Group lifecycle and disposal apply. Use Prop_First, Prop_Entries
*       and Prop_Visible through Group forwarding to control the range. There is
*       no separate public Prop-child getter.
*
*   METHODS
*       Construction creates Prop before Group and temporarily changes the first
*       caller ShowMe tag to TAG_IGNORE while creating Prop, then restores it.
*       The tag list must be writable. Additional repeated ShowMe tags are not
*       filtered by that single FindTagItem operation.
*       Setup calls Group first. For Type Default outside a window border, it
*       reorders the three children according to the application's scrollbar
*       arrangement. Its scrollbar-style preference switch currently makes no
*       changes. Setup returns FALSE on Group failure and TRUE otherwise,
*       ignoring sort results. Do not add unrelated children: the default sort
*       enumerates only the three internal objects.
*
*   BUGS
*       If Group construction fails, the already created Prop is not explicitly
*       disposed here. Prop and arrow allocation failures are not safely
*       rejected before Group addition, whose helper expects non-NULL children.
*       An unsupported Type leaves created objects unattached because the
*       insertion switch has no default branch. Thus a non-NULL result is not
*       proof of a complete usable control.
*       Forwarding and child-get fallback are inherited from Group; they are not
*       independent storage for Prop attributes. Keep the internal children and
*       their notification targets intact.
*
*/
/****** Scrollbar.mui/MUIA_Scrollbar_Type ************
*
*   NAME
*       MUIA_Scrollbar_Type
*
*   FUNCTION
*       Choose the ordering of Prop and arrow children. Default is
*       MUIV_Scrollbar_Type_Default.
*
*   INPUTS
*       LONG; initialize only. Use Default, Top, Bottom or Sym.
*
*   NOTES
*       Top initially orders Prop, decrease arrow, increase arrow. Bottom orders
*       decrease arrow, increase arrow, Prop. Sym orders decrease arrow, Prop,
*       increase arrow. Default initially uses Top order, then Setup applies the
*       application arrangement preference. Explicit types bypass that
*       preference reorder. These are actual list orders; names must not be
*       interpreted as a different visual ordering. With Prop_UseWinBorder, only
*       Prop is attached and this order is irrelevant. No local setter or getter
*       exists.
*
*   BUGS
*       Values outside the four enum constants are not rejected and select no
*       insertion branch outside border mode. Created objects can remain
*       unattached and undisposed.
*
*/
/****** Scrollbar.mui/MUIA_Scrollbar_IncDecSize ************
*
*   NAME
*       MUIA_Scrollbar_IncDecSize
*
*   FUNCTION
*       Set the amount argument passed by arrow Timer notifications to
*       Prop_Increase and Prop_Decrease. Default one.
*
*   INPUTS
*       ULONG; initialize, set and get. Getter writes the stored amount to IPTR
*       storage.
*
*   NOTES
*       Setting rebuilds each existing arrow Timer notification: KillNotify
*       removes notifications for Timer, then a new increase/decrease
*       notification is registered. Results are ignored. Zero and large amounts
*       are accepted without validation. Prop applies its own deltafactor and
*       bounds; the amount is not an unconditional pixel count. In border mode
*       there are no arrow Image objects, so this attribute is stored but does
*       not configure the window arrow gadgets.
*
*   BUGS
*       A getter value confirms stored state, not successful notification
*       registration. Prop arithmetic can overflow for large amount/factor
*       combinations; no extra validation is provided here.
*
*/

extern struct Library *MUIMasterBase;

struct Scrollbar_DATA
{
    Object *prop;
    Object *up_arrow;
    Object *down_arrow;
    int sb_pos;
    ULONG incdec;
};


/*
 * Create Prop while temporarily suppressing the first ShowMe tag, restore
 * it, then construct Group and create/attach arrows unless using a window
 * border. Requires writable tags and supported Type. Return object or zero
 * on Group failure, but do not explicitly free the earlier Prop on that
 * path. Child allocation failures are unchecked before attachment;
 * unsupported Type can leave children unattached.
 */
IPTR Scrollbar__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Scrollbar_DATA *data;
    //struct TagItem *tags,*tag;
    int horiz = GetTagData(MUIA_Group_Horiz, 0, msg->ops_AttrList);
    int usewinborder =
        GetTagData(MUIA_Prop_UseWinBorder, 0, msg->ops_AttrList);
    int sb_pos = GetTagData(MUIA_Scrollbar_Type, 0, msg->ops_AttrList);
    ULONG incdec =
        GetTagData(MUIA_Scrollbar_IncDecSize, 1, msg->ops_AttrList);

    /*
     * Filter MUIA_ShowMe from the tag list passed to the Prop child.
     * If a subclass sets MUIA_ShowMe=FALSE on the Scrollbar group,
     * it must not leak to the Prop, otherwise the prop knob stays
     * hidden even after the group is shown.
    */
    struct TagItem *showTag = FindTagItem(MUIA_ShowMe, msg->ops_AttrList);
    if (showTag)
        showTag->ti_Tag = TAG_IGNORE;

    Object *prop =
        MUI_NewObject(MUIC_Prop, PropFrame, MUIA_Prop_Horiz, horiz,
        TAG_MORE, (IPTR)msg->ops_AttrList);

    /* Restore the original tag so the Group (superclass) still sees it */
    if (showTag)
        showTag->ti_Tag = MUIA_ShowMe;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Spacing, 0,
        MUIA_Background, MUII_GroupBack,
        TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->prop = prop;
    data->sb_pos = sb_pos;
    data->incdec = incdec;

    if (!usewinborder)
    {
        data->up_arrow = ImageObject,
            MUIA_Background, MUII_ButtonBack,
            MUIA_Weight, 0,
            ImageButtonFrame,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Image_Spec, horiz ? MUII_ArrowLeft : MUII_ArrowUp,
            MUIA_Image_Prop, prop, End;
        if (data->up_arrow)
        {
            DoMethod(data->up_arrow, MUIM_Notify, MUIA_Timer,
                MUIV_EveryTime, (IPTR) prop, 2,
                MUIM_Prop_Decrease, incdec);
        }

        data->down_arrow = ImageObject,
            MUIA_Background, MUII_ButtonBack,
            MUIA_Weight, 0,
            ImageButtonFrame,
            MUIA_InputMode, MUIV_InputMode_RelVerify,
            MUIA_Image_Spec, horiz ? MUII_ArrowRight : MUII_ArrowDown,
            MUIA_Image_Prop, prop, End;
        if (data->down_arrow)
        {
            DoMethod(data->down_arrow, MUIM_Notify, MUIA_Timer,
                MUIV_EveryTime, (IPTR) prop, 2,
                MUIM_Prop_Increase, incdec);
        }

        switch (sb_pos)
        {
        case MUIV_Scrollbar_Type_Default:
        case MUIV_Scrollbar_Type_Top:
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->prop);
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->up_arrow);
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->down_arrow);
            break;
        case MUIV_Scrollbar_Type_Bottom:
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->up_arrow);
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->down_arrow);
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->prop);
            break;
        case MUIV_Scrollbar_Type_Sym:
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->up_arrow);
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->prop);
            DoMethod(obj, OM_ADDMEMBER, (IPTR) data->down_arrow);
            break;
        }
    }
    else
    {
        _flags(obj) |= MADF_BORDERGADGET;
        DoMethod(obj, OM_ADDMEMBER, (IPTR) data->prop);
    }

    return (IPTR) obj;
}


/*
 * Store every IncDecSize tag, remove each existing arrow Timer notification
 * and register its replacement with the new amount. Then return Group
 * setter result. Ignore notification results; zero and overflow-prone
 * values are not rejected. Requires live child targets and writable tag
 * list; Group can mutate forwarded tags.
 */
IPTR Scrollbar__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Scrollbar_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        if (tag->ti_Tag == MUIA_Scrollbar_IncDecSize)
        {
            data->incdec = tag->ti_Data;
            if (data->up_arrow)
            {
                DoMethod(data->up_arrow, MUIM_KillNotify, MUIA_Timer);
                DoMethod(data->up_arrow, MUIM_Notify, MUIA_Timer,
                    MUIV_EveryTime, (IPTR) data->prop, 2,
                    MUIM_Prop_Decrease, data->incdec);
            }
            if (data->down_arrow)
            {
                DoMethod(data->down_arrow, MUIM_KillNotify, MUIA_Timer);
                DoMethod(data->down_arrow, MUIM_Notify, MUIA_Timer,
                    MUIV_EveryTime, (IPTR) data->prop, 2,
                    MUIM_Prop_Increase, data->incdec);
            }
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/*
 * Write stored IncDecSize to caller IPTR storage and return one, otherwise
 * return Group getter result. Stored amount does not certify installed
 * notifications. No ownership transfer or local Type getter.
 */
IPTR Scrollbar__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Scrollbar_DATA *data = INST_DATA(cl, obj);

    if (msg->opg_AttrID == MUIA_Scrollbar_IncDecSize)
    {
        *(msg->opg_Storage) = data->incdec;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}


/*
 * Set up Group; return FALSE on failure. For Default nonborder controls
 * reorder the three internal children from the arrangement preference,
 * ignoring result. The style switch has no operative branches. Requires
 * valid connected preferences and intact child set. Return TRUE on
 * completion; do not add unrelated members that the sort would omit.
 */
IPTR Scrollbar__MUIM_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    struct Scrollbar_DATA *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, msg))
        return FALSE;

    if (!(_flags(obj) & MADF_BORDERGADGET) && !data->sb_pos)
    {
        switch (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_arrangement)
        {
        case SCROLLBAR_ARRANGEMENT_TOP:
            DoMethod(obj, MUIM_Group_Sort, (IPTR) data->prop,
                (IPTR) data->up_arrow, (IPTR) data->down_arrow,
                (IPTR) NULL);
            break;
        case SCROLLBAR_ARRANGEMENT_MIDDLE:
            DoMethod(obj, MUIM_Group_Sort, (IPTR) data->up_arrow,
                (IPTR) data->prop, (IPTR) data->down_arrow, (IPTR) NULL);
            break;
        case SCROLLBAR_ARRANGEMENT_BOTTOM:
            DoMethod(obj, MUIM_Group_Sort, (IPTR) data->up_arrow,
                (IPTR) data->down_arrow, (IPTR) data->prop, (IPTR) NULL);
            break;
        }

        switch (muiGlobalInfo(obj)->mgi_Prefs->scrollbar_type)
        {
        case SCROLLBAR_TYPE_STANDARD:
            break;
        case SCROLLBAR_TYPE_NEWLOOK:
            break;
        case SCROLLBAR_TYPE_CUSTOM:
            break;
        }
    }

    return TRUE;
}


/*
 * Route constructor, setter, getter and Setup overrides; delegate remaining
 * messages to Group. Return selected handler result. Requires valid
 * class/object and ABI-correct message; Group owns attached children and
 * their cleanup.
 */
BOOPSI_DISPATCHER(IPTR, Scrollbar_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Scrollbar__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_SET:
        return Scrollbar__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Scrollbar__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Scrollbar__MUIM_Setup(cl, obj, msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Scrollbar_desc =
{
    MUIC_Scrollbar,
    MUIC_Group,
    sizeof(struct Scrollbar_DATA),
    (void *)Scrollbar_Dispatcher
};
