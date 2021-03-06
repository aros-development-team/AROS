/*
    Copyright (C) 2002-2016, The AROS Development Team. All rights reserved.
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
};

/****** Listview.mui/MUIA_Listview_DefClickColumn ****************************
*
*   NAME
*       MUIA_Listview_DefClickColumn -- (V7) [ISG], LONG
*
*   FUNCTION
*       The default value to be returned for MUIA_Listview_ClickColumn when a
*       list entry is "double clicked" by pressing the "press" key (usually
*       return/enter). The default default is zero.
*
*   SEE ALSO
*       MUIA_Listview_ClickColumn
*
******************************************************************************
*
*/

/****** Listview.mui/MUIA_Listview_DoubleClick *******************************
*
*   NAME
*       MUIA_Listview_DoubleClick -- (V4) [..G], BOOL
*
*   FUNCTION
*       Set to TRUE when a list entry is double-clicked or the "press" key
*       (usually return/enter) is received.
*
*   SEE ALSO
*       MUIA_Listview_ClickColumn, MUIA_Listview_DefClickColumn,
*       MUIA_Listview_SelectChange
*
******************************************************************************
*
*/

/****** Listview.mui/MUIA_Listview_DragType **********************************
*
*   NAME
*       MUIA_Listview_DragType -- (V11) [ISG], LONG
*
*   FUNCTION
*       Specifies whether list entries can be dragged as part of a
*       drag-and-drop operation. Two values can be used:
*           MUIV_Listview_DragType_None: entries cannot be dragged
*           MUIV_Listview_DragType_Immediate: entries can be dragged
*
*   SEE ALSO
*       MUIA_Listview_DragSortable, MUIA_Draggable
*
******************************************************************************
*
*/

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
IPTR Listview__MUIM_KillNotify(struct IClass *cl, Object *obj,
    struct MUIP_KillNotify *msg)
{
    return RedirectNotify(cl, obj, (Msg) msg, msg->TrigAttr);
}

/**************************************************************************
 MUIM_KillNotifyObj
**************************************************************************/
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
