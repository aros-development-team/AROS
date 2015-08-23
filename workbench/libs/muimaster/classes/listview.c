/*
    Copyright © 2002-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>

#include <clib/alib_protos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"

extern struct Library *MUIMasterBase;

struct MUI_ListviewData
{
    Object *list;
    struct Hook selfnotify_hook;
    BOOL noforward;
};

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

    data->selfnotify_hook.h_Entry = HookEntry;
    data->selfnotify_hook.h_SubEntry = (HOOKFUNC) SelfNotify_Function;
    data->selfnotify_hook.h_Data = data;
    data->noforward = FALSE;

    /* parse initial taglist, forward to list */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Listview_DoubleClick:
            case MUIA_Listview_Input:
            case MUIA_Listview_MultiSelect:
            case MUIA_Listview_ScrollerPos:
                set(list, tag->ti_Tag, tag->ti_Data);
                break;
        }
    }

    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->selfnotify_hook,
        MUIA_List_Active, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->selfnotify_hook,
        MUIA_Listview_DoubleClick, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_Listview_SelectChange, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->selfnotify_hook,
        MUIA_Listview_SelectChange, MUIV_TriggerValue);
    DoMethod(list, MUIM_Notify, MUIA_Listview_ClickColumn, MUIV_EveryTime,
        (IPTR) obj, 4, MUIM_CallHook, (IPTR) &data->selfnotify_hook,
        MUIA_Listview_ClickColumn, MUIV_TriggerValue);

    return (IPTR) obj;
}

/**************************************************************************
 OM_SET
**************************************************************************/
IPTR Listview__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tag, *tags;
    IPTR no_notify = GetTagData(MUIA_NoNotify, FALSE, msg->ops_AttrList);
    struct MUI_ListviewData *data = INST_DATA(cl, obj);

    if (data->noforward)
    {
        return DoSuperMethodA(cl, obj, (Msg) msg);
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
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
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
    case MUIA_Listview_ClickColumn:
    case MUIA_Listview_DoubleClick:
    case MUIA_Listview_SelectChange:
        return GetAttr(msg->opg_AttrID, data->list, msg->opg_Storage);

    case MUIA_Listview_List:
        STORE = (IPTR) data->list;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

BOOPSI_DISPATCHER(IPTR, Listview_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_SET:
        return Listview__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Listview__OM_GET(cl, obj, (struct opGet *)msg);
    case OM_NEW:
        return Listview__OM_NEW(cl, obj, (struct opSet *)msg);
    case MUIM_Notify:
    case MUIM_KillNotify:
    case MUIM_KillNotifyObj:
        {
            struct MUI_ListviewData *data = INST_DATA(cl, obj);

            DoMethodA(data->list, msg);
            break;
        }
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
