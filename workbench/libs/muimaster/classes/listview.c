/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/debug.h>

#include <clib/alib_protos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"

/****** Listview.mui/MUIC_Listview ************
*
*   NAME
*       MUIC_Listview
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Wrap a supplied List-compatible object in a horizontal Group. Listview
*       requires List and takes child ownership through Group; it creates
*       neither a default list nor a separate scroller. The AROS List class owns
*       and arranges its own scroller. Listview defaults horizontal inner
*       spacing and group spacing to zero.
*       The constructor forwards DefClickColumn, DragType, Input, MultiSelect
*       and ScrollerPos to the list using setters, in tag order. Other
*       list-specific initialization belongs in the supplied list's constructor.
*       There is no local Set override: ordinary Group forwarding carries later
*       tags to children unless disabled by Group_Forward. There is no local
*       replacement setter for List.
*       The attribute behavior below describes an ordinary AROS List child.
*       Subclasses can change the forwarded operations and their return values.
*       Keep the owned list alive while the wrapper uses it; dispose the wrapper
*       through its normal Group lifecycle rather than separately disposing the
*       attached list.
*
*   METHODS
*       The dispatcher forwards these methods directly to List and returns its
*       result: MUIM_List_Clear, CreateImage, DeleteImage, Exchange, GetEntry,
*       Insert, InsertSingle, Jump, NextSelected, Redraw, Remove, Select, Sort
*       and TestPos. The abbreviated names begin with MUIM_List followed by an
*       underscore. Other messages use Group; there is no blanket forwarding of
*       all List or custom methods.
*       Get forwards List_CompareHook, ConstructHook, DestructHook, DisplayHook,
*       VertProp_First, Format, VertProp_Entries, VertProp_Visible, Active,
*       First, Visible, Entries and Quiet, plus Listview_ClickColumn,
*       DefClickColumn, DoubleClick, DragType and SelectChange. The child getter
*       result is returned unchanged; listing an attribute here does not
*       guarantee that the child's getter implements it.
*       Notify, KillNotify and KillNotifyObj use the same attribute routing
*       table. They target the child for List_Active, AutoVisible, CompareHook,
*       ConstructHook, DestructHook, DisplayHook, DragSortable, DropMark,
*       Entries, First, Format, InsertPosition, MultiTestHook, Quiet,
*       ShowDropMarks, Title, VertProp_Entries, VertProp_First, VertProp_Visible
*       and Visible; also Listview_ClickColumn, DoubleClick and SelectChange.
*       Other trigger attributes use Group/Notify on the wrapper. The
*       abbreviated names begin with MUIA followed by an underscore.
*       Notify and KillNotifyObj replace MUIV_Notify_Self with the actual
*       wrapper pointer in the caller's writable message before routing. Self
*       therefore remains the Listview even when the notification record belongs
*       to List. Registration/removal results come from the selected receiver.
*       Notification callbacks are synchronous; do not remove an active record
*       or dispose its source/destination during dispatch.
*
*/
/****** Listview.mui/MUIA_Listview_List ************
*
*   NAME
*       MUIA_Listview_List
*
*   FUNCTION
*       Supply or obtain the list object owned by the wrapper.
*
*   INPUTS
*       Initialize and get, Object pointer. Required and non-NULL; no default or
*       local replacement setter.
*
*   NOTES
*       Use a live, unattached List-compatible child. Successful Group
*       attachment transfers disposal responsibility to the wrapper. Get returns
*       the same borrowed pointer with TRUE; do not free it or dispose it
*       separately. Missing/NULL List returns NULL before superclass
*       construction. If later Group construction fails, follow Group
*       child-ownership rules; failure does not promise that the supplied child
*       remains undisposed.
*
*/
/****** Listview.mui/MUIA_Listview_ClickColumn ************
*
*   NAME
*       MUIA_Listview_ClickColumn
*
*   FUNCTION
*       Read the column recorded by the child list for its latest relevant
*       input.
*
*   INPUTS
*       Get, LONG; initially zero in a new AROS List. Getter and notification
*       registration are redirected to List.
*
*   NOTES
*       A valid interactive mouse entry press stores the TestPos column and
*       notifies before DoubleClick and before the later active-entry update.
*       Keyboard Press stores DefClickColumn and notifies before DoubleClick.
*       Keyboard Toggle also stores DefClickColumn but does not emit the same
*       ClickColumn tag. This is cached input context, not a continuous
*       pointer-position query. Read Active with care during mouse double-click
*       callbacks: the new active-entry assignment occurs later in the event
*       handler.
*
*/
/****** Listview.mui/MUIA_Listview_DefClickColumn ************
*
*   NAME
*       MUIA_Listview_DefClickColumn
*
*   FUNCTION
*       Choose the column used for keyboard activation and selection toggling.
*
*   INPUTS
*       Initialize, set and get, LONG. Default zero in a new AROS List. Listview
*       forwards the constructor/set value and the getter.
*
*   NOTES
*       No column-range validation is performed. List copies this value into
*       ClickColumn on keyboard Press before sending ClickColumn and DoubleClick
*       notifications; Toggle also stores it when multiselection is enabled.
*       Notifications registered for DefClickColumn itself remain on the wrapper
*       rather than being redirected to the child.
*
*/
/****** Listview.mui/MUIA_Listview_DoubleClick ************
*
*   NAME
*       MUIA_Listview_DoubleClick
*
*   FUNCTION
*       Report interactive double-click or keyboard activation events from the
*       child list.
*
*   INPUTS
*       Get and notification trigger, BOOL. Initially FALSE. Although the header
*       also marks initialize, Listview does not forward this constructor tag
*       and List does not parse it during construction.
*
*   NOTES
*       List internally sets TRUE for two timed mouse presses on the same valid
*       entry, or keyboard Press in input-enabled mode. Keyboard Press does not
*       require an active entry. The stored value remains TRUE until explicitly
*       changed; it is not automatically reset or consumed by Get. Repeated TRUE
*       tags can deliver repeated notifications. Use notifications for events,
*       not polling for a fresh transition.
*       AROS List also accepts this tag in Set, despite its nonpublic setter
*       role; normal Group forwarding can reach it. Setting a value does not
*       simulate an actual input event or update ClickColumn. Read-only input
*       remaps Press to scrolling. Mouse activation notifies before the event
*       handler performs its later Active update.
*
*/
/****** Listview.mui/MUIA_Listview_DragType ************
*
*   NAME
*       MUIA_Listview_DragType
*
*   FUNCTION
*       Control whether dragging uses an entry drag image.
*
*   INPUTS
*       Initialize, set and get, LONG. Declared values: None (0, default) and
*       Immediate (1).
*
*   NOTES
*       AROS List treats any nonzero value as entry dragging and enables
*       MUIA_Draggable when this attribute is set; zero disables that flag
*       through this setter. With drag type None, the drag-image method
*       delegates to its superclass, allowing whole-list dragging if Draggable
*       is enabled separately. DragSortable is a separate List attribute and is
*       not enabled by DragType. The correct identifier is
*       MUIA_List_DragSortable. Listview forwards the getter, but notifications
*       on DragType itself remain on the wrapper.
*
*/
/****** Listview.mui/MUIA_Listview_Input ************
*
*   NAME
*       MUIA_Listview_Input
*
*   FUNCTION
*       Enable selection-oriented user input or read-only scrolling.
*
*   INPUTS
*       Initialize, BOOL; default TRUE for a new AROS List. Listview forwards it
*       to the child. No dedicated getter in Listview or List.
*
*   NOTES
*       FALSE sets List read_only. Mouse entry presses then do not select or
*       double-click entries. Keyboard Up/Down, Top/Bottom, Press and page keys
*       use scrolling paths instead of the normal selection/activation paths.
*       Programmatic selection and list mutation remain possible; this is not a
*       disabled-object or data-protection flag.
*       AROS List has an internal setter, and Group forwarding also permits
*       later changes through Listview. That setter only updates read_only; it
*       does not cancel an already active drag-selection event sequence or clear
*       existing selections. Prefer initialization or change it while input
*       processing is idle.
*
*/
/****** Listview.mui/MUIA_Listview_MultiSelect ************
*
*   NAME
*       MUIA_Listview_MultiSelect
*
*   FUNCTION
*       Choose the child list multiselection policy.
*
*   INPUTS
*       Initialize, LONG. None (0, default), Default (1), Shifted (2) or Always
*       (3). No dedicated getter in Listview or List.
*
*   NOTES
*       None disables interactive multiselection without disabling the active
*       entry. Shifted clears existing selections on an unshifted entry press
*       and toggles with Shift; Always toggles without requiring Shift. Drag
*       selection extends the current selection operation. A MultiTestHook on
*       List can further restrict selection.
*       Default is resolved during List Setup from its preferences to Shifted or
*       Always, replacing the stored mode. It is not automatically re-resolved
*       on later setups unless reset to Default. AROS accepts later values
*       through its internal setter, reached by Group forwarding, but that
*       setter does not itself resolve Default. Avoid unknown values: nonzero
*       tests and exact Shifted comparisons do not validate the enumeration.
*
*/
/****** Listview.mui/MUIA_Listview_ScrollerPos ************
*
*   NAME
*       MUIA_Listview_ScrollerPos
*
*   FUNCTION
*       Choose the vertical scroller placement inside the child List.
*
*   INPUTS
*       Initialize; enumeration despite the header BOOL annotation. Default (0),
*       Left (1), Right (2) or None (3). No dedicated getter in Listview or
*       List.
*
*   NOTES
*       AROS List places Default and Right on the right; Left precedes the list
*       area and None removes the scroller. Other values follow the right-hand
*       default branch, not a preference lookup.
*       Listview forwards initialization using a setter before normal setup. The
*       List internal setter can rearrange children only while List is not set
*       up. After Setup it stores the new value but the arrangement helper
*       returns without rearranging; Setup itself does not retry that helper. A
*       later pre-setup setter is required to apply a changed arrangement. Use
*       the constructor tag for normal operation.
*
*/
/****** Listview.mui/MUIA_Listview_SelectChange ************
*
*   NAME
*       MUIA_Listview_SelectChange
*
*   FUNCTION
*       Notify that input processing may have changed selected entries.
*
*   INPUTS
*       Get and notification trigger, BOOL. Listview redirects both to List.
*
*   NOTES
*       The List getter always returns FALSE. The event handler emits TRUE after
*       operations that may alter multiselection, based on its
*       clear/toggle/select decisions rather than a comparison of the final
*       selection set. A TRUE notification does not guarantee that membership
*       actually changed. This is not a stored dirty flag, not an Active-change
*       notification and not a universal notification for every programmatic
*       MUIM_List_Select call.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_ListviewData
{
    Object *list;
};




/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Require a live unattached List-compatible child, create owning horizontal
 * Group, then forward the five selected configuration tags to child in
 * original order. Return object or NULL on missing child/superclass
 * failure; setter results ignored. No default List, separate scroller or
 * replacement mechanism.
 */
IPTR Listview__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListviewData *data;
    struct TagItem *tag, *tags;
    Object *list =
        (Object *) GetTagData(MUIA_Listview_List, (IPTR) NULL,
        msg->ops_AttrList);

    if (!list)
        return (IPTR) NULL;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Horiz, TRUE,
        MUIA_InnerLeft, 0,
        MUIA_InnerRight, 0,
        MUIA_Group_Spacing, 0,
        Child, list,
        TAG_MORE, msg->ops_AttrList);

    if (!obj)
        return (IPTR) NULL;

    data = INST_DATA(cl, obj);
    data->list = list;

    /* parse initial taglist, forward to list */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Listview_DefClickColumn:
            case MUIA_Listview_DragType:
            case MUIA_Listview_Input:
            case MUIA_Listview_MultiSelect:
            case MUIA_Listview_ScrollerPos:
                set(list, tag->ti_Tag, tag->ti_Data);
                break;
        }
    }

    return (IPTR) obj;
}

/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Return borrowed owned List pointer with TRUE, forward the enumerated list
 * getters and their statuses to child, or delegate other attributes to
 * Group. Requires writable storage and live owned child. Forwarding does
 * not guarantee getter support or extend pointer lifetime.
 */
IPTR Listview__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
/* small macro to simplify return value storage */
#define STORE *(msg->opg_Storage)
    struct MUI_ListviewData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_List_CompareHook:
    case MUIA_List_ConstructHook:
    case MUIA_List_DestructHook:
    case MUIA_List_DisplayHook:
    case MUIA_List_VertProp_First:
    case MUIA_List_Format:
    case MUIA_List_VertProp_Entries:
    case MUIA_List_VertProp_Visible:
    case MUIA_List_Active:
    case MUIA_List_First:
    case MUIA_List_Visible:
    case MUIA_List_Entries:
    case MUIA_List_Quiet:
    case MUIA_Listview_ClickColumn:
    case MUIA_Listview_DefClickColumn:
    case MUIA_Listview_DoubleClick:
    case MUIA_Listview_DragType:
    case MUIA_Listview_SelectChange:
        return GetAttr(msg->opg_AttrID, data->list, msg->opg_Storage);

    case MUIA_Listview_List:
        STORE = (IPTR) data->list;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/*
 * Route Notify/KillNotify/KillNotifyObj for the explicit list trigger set
 * to the owned child, other attributes to Group; return selected receiver
 * status. Requires ABI-correct writable message and live child. This
 * routing table differs from getter routing and does not cover every List
 * attribute.
 */
static IPTR RedirectNotify(struct IClass *cl, Object *obj, Msg msg,
    ULONG attr)
{
    struct MUI_ListviewData *data = INST_DATA(cl, obj);

    switch (attr)
    {
    case MUIA_List_Active:
    case MUIA_List_AutoVisible:
    case MUIA_List_CompareHook:
    case MUIA_List_ConstructHook:
    case MUIA_List_DestructHook:
    case MUIA_List_DisplayHook:
    case MUIA_List_DragSortable:
    case MUIA_List_DropMark:
    case MUIA_List_Entries:
    case MUIA_List_First:
    case MUIA_List_Format:
    case MUIA_List_InsertPosition:
    case MUIA_List_MultiTestHook:
    case MUIA_List_Quiet:
    case MUIA_List_ShowDropMarks:
    case MUIA_List_Title:
    case MUIA_List_VertProp_Entries:
    case MUIA_List_VertProp_First:
    case MUIA_List_VertProp_Visible:
    case MUIA_List_Visible:
    case MUIA_Listview_ClickColumn:
    case MUIA_Listview_DoubleClick:
    case MUIA_Listview_SelectChange:
        return DoMethodA(data->list, msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 MUIM_Notify
**************************************************************************/
/*
 * Replace Self destination in caller message with wrapper pointer, then
 * route by trigger attribute and return registration result. Requires
 * writable message, valid notification parameters and live destination.
 * Child-owned registration retains wrapper destination, not an ownership
 * reference.
 */
IPTR Listview__MUIM_Notify(struct IClass *cl, Object *obj,
    struct MUIP_Notify *msg)
{
    if ((IPTR)msg->DestObj == MUIV_Notify_Self)
        msg->DestObj = obj;

    return RedirectNotify(cl, obj, (Msg) msg, msg->TrigAttr);
}

/**************************************************************************
 MUIM_KillNotify
**************************************************************************/
/*
 * Route removal for the trigger attribute to the same child/Group receiver
 * used by registration, returning its result. Requires live object and ABI-
 * correct message; must not delete a notification record currently being
 * dispatched.
 */
IPTR Listview__MUIM_KillNotify(struct IClass *cl, Object *obj,
    struct MUIP_KillNotify *msg)
{
    return RedirectNotify(cl, obj, (Msg) msg, msg->TrigAttr);
}

/**************************************************************************
 MUIM_KillNotifyObj
**************************************************************************/
/*
 * Replace Self destination in caller message with wrapper pointer, then
 * route matching destination removal and return receiver status. Requires
 * writable message and live wrapper/child; do not remove an active dispatch
 * record.
 */
IPTR Listview__MUIM_KillNotifyObj(struct IClass *cl, Object *obj,
    struct MUIP_KillNotifyObj *msg)
{
    if ((IPTR)msg->dest == MUIV_Notify_Self)
        msg->dest = obj;

    return RedirectNotify(cl, obj, (Msg) msg, msg->TrigAttr);
}

/* Note that there is no OM_SET method here because this method will be
   propagated to the list by the Group class (superclass of both List and
   Listview) */
/*
 * Dispatch local getters/construction/notification routing and the
 * enumerated fourteen List methods. Return child result for those methods;
 * delegate other messages to Group. Requires live owned child except
 * construction and correct method message. Set uses inherited Group
 * forwarding, not a local handler.
 */
BOOPSI_DISPATCHER(IPTR, Listview_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_GET:
        return Listview__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_NEW:
        return Listview__OM_NEW(cl, obj, (struct opSet *)msg);
    case MUIM_Notify:
        return Listview__MUIM_Notify(cl, obj, (struct MUIP_Notify *)msg);
    case MUIM_KillNotify:
        return Listview__MUIM_KillNotify(cl, obj,
            (struct MUIP_KillNotify *)msg);
    case MUIM_KillNotifyObj:
        return Listview__MUIM_KillNotifyObj(cl, obj,
            (struct MUIP_KillNotifyObj *)msg);
    case MUIM_List_Clear:
    case MUIM_List_CreateImage:
    case MUIM_List_DeleteImage:
    case MUIM_List_Exchange:
    case MUIM_List_GetEntry:
    case MUIM_List_Insert:
    case MUIM_List_InsertSingle:
    case MUIM_List_Jump:
    case MUIM_List_NextSelected:
    case MUIM_List_Redraw:
    case MUIM_List_Remove:
    case MUIM_List_Select:
    case MUIM_List_Sort:
    case MUIM_List_TestPos:
        {
            struct MUI_ListviewData *data = INST_DATA(cl, obj);

            return DoMethodA(data->list, msg);
        }

    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Listview_desc =
{
    MUIC_Listview,
    MUIC_Group,
    sizeof(struct MUI_ListviewData),
    (void *) Listview_Dispatcher
};
