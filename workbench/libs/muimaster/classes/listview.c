/*
    Copyright © 2002-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <devices/rawkeycodes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "window.h"
#include "list.h"
#include "prefs.h"

extern struct Library *MUIMasterBase;

struct MUI_ListviewData
{
    Object *list, *group, *vert;
    struct Hook hook;
    struct Hook selfnotify_hook;
    BOOL noforward;
    BOOL read_only;
    IPTR multiselect;
    IPTR scroller_pos;
    struct MUI_EventHandlerNode ehn;

    int mouse_click;            /* see below if mouse is held down */

    /* double click */
    ULONG last_secs;
    ULONG last_mics;
    ULONG last_active;
    BOOL doubleclick;

    /* clicked column */
    LONG click_column;
    LONG def_click_column;

    /* user prefs */
    ListviewMulti prefs_multi;
};

#define MOUSE_CLICK_ENTRY 1     /* on entry clicked */
#define MOUSE_CLICK_TITLE 2     /* on title clicked */

static void DoWheelMove(struct IClass *cl, Object *obj, LONG wheely);

#define PROP_VERT_FIRST   1
#define LIST_VERT_FIRST   4
#define LIST_VERT_VISIBLE 5
#define LIST_VERT_ENTRIES 6

ULONG Listview_Function(struct Hook *hook, APTR dummyobj, void **msg)
{
    struct MUI_ListviewData *data = (struct MUI_ListviewData *)hook->h_Data;
    SIPTR type = (SIPTR) msg[0];
    SIPTR val = (SIPTR) msg[1];

    D(bug("[ListView] List 0x%p, Event %d, value %ld\n", data->list, type,
            val));

    switch (type)
    {
    case PROP_VERT_FIRST:
        get(data->vert, MUIA_Prop_First, &val);
        nnset(data->list, MUIA_List_VertProp_First, val);
        break;

    case LIST_VERT_FIRST:
        nnset(data->vert, MUIA_Prop_First, val);
        break;
    case LIST_VERT_VISIBLE:
        nnset(data->vert, MUIA_Prop_Visible, val);
        break;
    case LIST_VERT_ENTRIES:
        nnset(data->vert, MUIA_Prop_Entries, val);
        break;
    }
    return 0;
}

ULONG SelfNotify_Function(struct Hook *hook, APTR obj, void **msg)
{
    struct MUI_ListviewData *data = (struct MUI_ListviewData *)hook->h_Data;
    SIPTR attribute = (SIPTR) msg[0];
    SIPTR value = (SIPTR) msg[1];

    /* This allows avoiding notify loops */
    data->noforward = TRUE;
    SetAttrs(obj, MUIA_Group_Forward, FALSE, attribute, value, TAG_DONE);
    data->noforward = FALSE;

    return 0;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
IPTR Listview__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_ListviewData *data;
    struct TagItem *tag, *tags;
    Object *group, *vert;
    Object *list =
        (Object *) GetTagData(MUIA_Listview_List, (IPTR) NULL,
        msg->ops_AttrList);
    IPTR cyclechain =
        (IPTR) GetTagData(MUIA_CycleChain, (IPTR) 0, msg->ops_AttrList);
    LONG entries = 0, first = 0, visible = 0;

    if (!list)
        return (IPTR) NULL;

    vert = ScrollbarObject, MUIA_Group_Horiz, FALSE, End;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Horiz, FALSE,
        MUIA_CycleChain, cyclechain,
        MUIA_InnerLeft, 0,
        MUIA_InnerRight, 0,
        Child, (IPTR) (group = HGroup,
            MUIA_InnerLeft, 0,
            MUIA_InnerRight, 0,
            MUIA_Group_Spacing, 0,
            End),
        TAG_DONE);

    if (!obj)
        return (IPTR) NULL;

    data = INST_DATA(cl, obj);
    data->list = list;
    data->vert = vert;
    data->group = group;

    data->hook.h_Entry = HookEntry;
    data->hook.h_SubEntry = (HOOKFUNC) Listview_Function;
    data->hook.h_Data = data;

    data->selfnotify_hook.h_Entry = HookEntry;
    data->selfnotify_hook.h_SubEntry = (HOOKFUNC) SelfNotify_Function;
    data->selfnotify_hook.h_Data = data;
    data->noforward = FALSE;

    data->last_active = -1;

    data->ehn.ehn_Events = IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Listview_DoubleClick:
                data->doubleclick = tag->ti_Data != 0;
                break;
            case MUIA_Listview_Input:
                data->read_only = !tag->ti_Data;
                break;
            case MUIA_Listview_MultiSelect:
                data->multiselect = tag->ti_Data;
                break;
            case MUIA_Listview_ScrollerPos:
                data->scroller_pos = tag->ti_Data;
                break;
        }
    }

    /* Add list and/or scroller */
    switch (data->scroller_pos)
    {
    case MUIV_Listview_ScrollerPos_None:
        DoMethod(group, OM_ADDMEMBER, list);
        break;
    case MUIV_Listview_ScrollerPos_Left:
        DoMethod(group, OM_ADDMEMBER, vert);
        DoMethod(group, OM_ADDMEMBER, list);
        break;
    default:
        DoMethod(group, OM_ADDMEMBER, list);
        DoMethod(group, OM_ADDMEMBER, vert);
        break;
    }

    get(list, MUIA_List_VertProp_First, &first);
    get(list, MUIA_List_VertProp_Visible, &visible);
    get(list, MUIA_List_VertProp_Entries, &entries);

    D(bug
        ("[ListView 0x%p] List 0x%p, First %ld, Visible %ld, Entries %ld\n",
            obj, list, first, visible, entries));

    SetAttrs(data->vert,
        MUIA_Prop_First, first,
        MUIA_Prop_Visible, visible, MUIA_Prop_Entries, entries, TAG_DONE);

    DoMethod(vert, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, (IPTR) obj,
        4, MUIM_CallHook, (IPTR) &data->hook, PROP_VERT_FIRST,
        MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_First, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->hook, LIST_VERT_FIRST,
        MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_Visible, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->hook,
        LIST_VERT_VISIBLE, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_VertProp_Entries, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->hook,
        LIST_VERT_ENTRIES, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->selfnotify_hook,
        MUIA_List_Active, MUIV_TriggerValue);

    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
IPTR Listview__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
void Listview__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tag, *tags;
    IPTR no_notify = GetTagData(MUIA_NoNotify, FALSE, msg->ops_AttrList);
    struct MUI_ListviewData *data = INST_DATA(cl, obj);

    if (data->noforward)
    {
        DoSuperMethodA(cl, obj, (Msg) msg);
        return;
    }

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
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
            SetAttrs(data->list, MUIA_NoNotify, no_notify, tag->ti_Tag,
                tag->ti_Data, TAG_DONE);
            break;
        case MUIA_Listview_DoubleClick:
            data->doubleclick = tag->ti_Data != 0;
            break;
        }
    }
}

/**************************************************************************
 OM_GET
**************************************************************************/
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
        return GetAttr(msg->opg_AttrID, data->list, msg->opg_Storage);
    case MUIA_Listview_DoubleClick:
        STORE = data->doubleclick;
        return 1;
    case MUIA_Listview_ClickColumn:
        STORE = data->click_column;
        return 1;
    case MUIA_Listview_List:
        STORE = (IPTR) data->list;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
IPTR Listview__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_ListviewData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return 0;

    data->prefs_multi = muiGlobalInfo(obj)->mgi_Prefs->list_multi;
    if (data->multiselect == MUIV_Listview_MultiSelect_Default)
    {
        if (data->prefs_multi == LISTVIEW_MULTI_SHIFTED)
            data->multiselect = MUIV_Listview_MultiSelect_Shifted;
        else
            data->multiselect = MUIV_Listview_MultiSelect_Always;
    }

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) &data->ehn);

    return 1;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
IPTR Listview__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_ListviewData *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) &data->ehn);
    data->mouse_click = 0;

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_HandleEvent
**************************************************************************/
IPTR Listview__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    struct MUI_ListviewData *data = INST_DATA(cl, obj);
    Object *list = data->list;
    struct MUI_List_TestPos_Result pos;
    LONG seltype, old_active, new_active, visible;
    IPTR result = 0;
    BOOL select = FALSE, clear = FALSE;
    WORD delta;
    typeof(msg->muikey) muikey = msg->muikey;

    new_active = old_active = XGET(list, MUIA_List_Active);
    visible = XGET(list, MUIA_List_Visible);

    if (muikey != MUIKEY_NONE)
    {
        result = MUI_EventHandlerRC_Eat;

        /* Make keys behave differently in read-only mode */
        if (data->read_only)
        {
            switch (muikey)
            {
            case MUIKEY_TOP:
                muikey = MUIKEY_LINESTART;
                break;

            case MUIKEY_BOTTOM:
                muikey = MUIKEY_LINEEND;
                break;

            case MUIKEY_UP:
                muikey = MUIKEY_LEFT;
                break;

            case MUIKEY_DOWN:
            case MUIKEY_PRESS:
                muikey = MUIKEY_RIGHT;
                break;
            }
        }

        switch (muikey)
        {
        case MUIKEY_TOGGLE:
            if (data->multiselect != MUIV_Listview_MultiSelect_None
                && !data->read_only)
            {
                select = TRUE;
                data->click_column = data->def_click_column;
                new_active = MUIV_List_Active_Down;
            }
            else
            {
                DoMethod(list, MUIM_List_Jump, 0);
                muikey = MUIKEY_NONE;
            }
            break;

        case MUIKEY_TOP:
            new_active = MUIV_List_Active_Top;
            break;

        case MUIKEY_BOTTOM:
            new_active = MUIV_List_Active_Bottom;
            break;

        case MUIKEY_LEFT:
        case MUIKEY_WORDLEFT:
            DoMethod(list, MUIM_List_Jump, MUIV_List_Jump_Up);
            break;

        case MUIKEY_RIGHT:
        case MUIKEY_WORDRIGHT:
            DoMethod(list, MUIM_List_Jump, MUIV_List_Jump_Down);
            break;

        case MUIKEY_LINESTART:
            DoMethod(list, MUIM_List_Jump, MUIV_List_Jump_Top);
            break;

        case MUIKEY_LINEEND:
            DoMethod(list, MUIM_List_Jump, MUIV_List_Jump_Bottom);
            break;

        case MUIKEY_UP:
            new_active = MUIV_List_Active_Up;
            break;

        case MUIKEY_DOWN:
            new_active = MUIV_List_Active_Down;
            break;

        case MUIKEY_PAGEUP:
            if (data->read_only)
                DoWheelMove(cl, obj, -visible);
            else
                new_active = MUIV_List_Active_PageUp;
            break;

        case MUIKEY_PAGEDOWN:
            if (data->read_only)
                DoWheelMove(cl, obj, visible);
            else
                new_active = MUIV_List_Active_PageDown;
            break;

        default:
            result = 0;
        }
    }
    else if (msg->imsg)
    {
        DoMethod(list, MUIM_List_TestPos, msg->imsg->MouseX,
            msg->imsg->MouseY, (IPTR) &pos);

        switch (msg->imsg->Class)
        {
        case IDCMP_MOUSEBUTTONS:
            if (msg->imsg->Code == SELECTDOWN)
            {
                if (_isinobject(list, msg->imsg->MouseX, msg->imsg->MouseY))
                {
                    data->mouse_click = MOUSE_CLICK_ENTRY;

                    if (!data->read_only)
                    {
                        new_active = pos.entry;

                        clear = (data->multiselect
                            == MUIV_Listview_MultiSelect_Shifted
                            && (msg->imsg->Qualifier
                            & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) == 0);
                        seltype = clear ?
                            MUIV_List_Select_On: MUIV_List_Select_Toggle;
                        select = data->multiselect
                            != MUIV_Listview_MultiSelect_None;

                        /* Handle MUIA_Listview_ClickColumn */
                        data->click_column = pos.column;

                        /* Handle double clicking */
                        if (data->last_active == pos.entry
                            && DoubleClick(data->last_secs, data->last_mics,
                                msg->imsg->Seconds, msg->imsg->Micros))
                        {
                            set(obj, MUIA_Listview_DoubleClick, TRUE);
                            data->last_active = -1;
                            data->last_secs = data->last_mics = 0;
                        }
                        else
                        {
                            data->last_active = pos.entry;
                            data->last_secs = msg->imsg->Seconds;
                            data->last_mics = msg->imsg->Micros;
                        }

                        /* Look out for mouse movement and timer events while
                           mouse button is down */
                        DoMethod(_win(list), MUIM_Window_RemEventHandler,
                            (IPTR) &data->ehn);
                        data->ehn.ehn_Events |=
                            (IDCMP_MOUSEMOVE | IDCMP_INTUITICKS);
                        DoMethod(_win(list), MUIM_Window_AddEventHandler,
                            (IPTR) &data->ehn);
                    }
                }
            }
            else
            {
                /* Activate object */
                if (msg->imsg->Code == SELECTUP && data->mouse_click)
                {
                    set(_win(obj), MUIA_Window_ActiveObject, (IPTR)obj);
                    data->mouse_click = 0;
                }

                /* Restore normal event mask */
                DoMethod(_win(list), MUIM_Window_RemEventHandler,
                    (IPTR) &data->ehn);
                data->ehn.ehn_Events &=
                    ~(IDCMP_MOUSEMOVE | IDCMP_INTUITICKS);
                DoMethod(_win(list), MUIM_Window_AddEventHandler,
                    (IPTR) &data->ehn);
            }
            break;

        case IDCMP_MOUSEMOVE:
        case IDCMP_INTUITICKS:
            if (pos.flags & MUI_LPR_ABOVE)
                new_active = MUIV_List_Active_Up;
            else if (pos.flags & MUI_LPR_BELOW)
                new_active = MUIV_List_Active_Down;
            else
                new_active = pos.entry;

            select = new_active != old_active
                && data->multiselect != MUIV_Listview_MultiSelect_None;
            if (select)
                DoMethod(list, MUIM_List_Select, MUIV_List_Select_Active,
                    MUIV_List_Select_Ask, &seltype);

            break;

        case IDCMP_RAWKEY:
            /* Scroll wheel */
            if (_isinobject(data->vert, msg->imsg->MouseX, msg->imsg->MouseY))
                delta = 1;
            else if (_isinobject(list, msg->imsg->MouseX, msg->imsg->MouseY))
                delta = 4;
            else
                delta = 0;

            if (delta != 0)
            {
                switch (msg->imsg->Code)
                {
                case RAWKEY_NM_WHEEL_UP:
                    DoWheelMove(cl, obj, -delta);
                    break;

                case RAWKEY_NM_WHEEL_DOWN:
                    DoWheelMove(cl, obj, delta);
                    break;
                }
                result = MUI_EventHandlerRC_Eat;
            }
            break;
        }
    }

    /* Change selected and active entries */
    if (clear)
    {
        DoMethod(list, MUIM_List_Select, MUIV_List_Select_All,
            MUIV_List_Select_Off, NULL);
    }

    if (muikey == MUIKEY_TOGGLE)
    {
        DoMethod(list, MUIM_List_Select,
            MUIV_List_Select_Active,
            MUIV_List_Select_Toggle, NULL);
        select = FALSE;
    }

    if (new_active != old_active)
        set(list, MUIA_List_Active, new_active);

    if (select)
        DoMethod(list, MUIM_List_Select, MUIV_List_Select_Active, seltype,
            NULL);

    return result;
}

static void DoWheelMove(struct IClass *cl, Object *obj, LONG wheely)
{
    struct MUI_ListviewData *data = INST_DATA(cl, obj);
    LONG new, first, entries, visible;

    new = first = XGET(data->list, MUIA_List_First);
    entries = XGET(data->list, MUIA_List_Entries);
    visible = XGET(data->list, MUIA_List_Visible);

    new += wheely;

    if (new > entries - visible)
    {
        new = entries - visible;
    }

    if (new < 0)
    {
        new = 0;
    }

    if (new != first)
    {
        set(data->list, MUIA_List_First, new);
    }
}

BOOPSI_DISPATCHER(IPTR, Listview_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_SET:
        Listview__OM_SET(cl, obj, (struct opSet *)msg);
        break;
    case OM_GET:
        return Listview__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_NEW:
        return Listview__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Listview__OM_DISPOSE(cl, obj, msg);
    case MUIM_Setup:
        return Listview__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Listview__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
    case MUIM_HandleEvent:
        return Listview__MUIM_HandleEvent(cl, obj,
            (struct MUIP_HandleEvent *)msg);
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
