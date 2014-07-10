/*
    Copyright © 2002-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "debug.h"

extern struct Library *MUIMasterBase;

struct Popobject_DATA
{
    BOOL light;
    BOOL vol;
    BOOL follow;
    BOOL popped;
    struct Hook *strobj_hook;
    struct Hook *objstr_hook;
    struct Hook *window_hook;
    struct Hook open_hook;
    struct Hook close_hook;
    Object *object;
    Object *wnd;
};

/****** Popobject.mui/MUIA_Popobject_Follow **********************************
*
*   NAME
*       MUIA_Popobject_Follow -- (V7) [ISG], BOOL
*
*   FUNCTION
*       Specifies whether the pop-up window should update its position and
*       width to match the position and width of its string gadget when the
*       parent window is moved or resized, i.e. if it should look like it is
*       attached to the string gadget, or should be more like a floating
*       object. Defaults to TRUE.
*
******************************************************************************
*
*/

/****** Popobject.mui/MUIA_Popobject_Light ***********************************
*
*   NAME
*       MUIA_Popobject_Light -- (V7) [ISG], BOOL
*
*   FUNCTION
*       Specifies whether the pop-up window should be drawn without a border.
*       Defaults to TRUE.
*
******************************************************************************
*
*/

/****** Popobject.mui/MUIA_Popobject_Object **********************************
*
*   NAME
*       MUIA_Popobject_Object -- (V7) [I.G], Object *
*
*   FUNCTION
*       The object to pop up.
*
******************************************************************************
*
*/

/****** Popobject.mui/MUIA_Popobject_ObjStrHook ******************************
*
*   NAME
*       MUIA_Popobject_ObjStrHook -- (V7) [ISG], struct Hook *
*
*   FUNCTION
*       This hook is called to update the string gadget when the pop-up is
*       closed with a TRUE success value. For example, it can set the string
*       to the name of the item selected from a Listview pop-up.
*
*       The hook receives pointers to the pop-up object and the string gadget
*       as its second and third arguments respectively.
*
*   SEE ALSO
*       MUIA_Popobject_StrObjHook
*
******************************************************************************
*
*/

/****** Popobject.mui/MUIA_Popobject_StrObjHook ******************************
*
*   NAME
*       MUIA_Popobject_StrObjHook -- (V7) [ISG], struct Hook *
*
*   FUNCTION
*       This hook is called whenever the pop-up object is opened to initialise
*       or update the pop-up object according to the contents of the string
*       gadget. For example, it can pre-select an item in a Listview pop-up
*       that matches the current string.
*
*       The hook receives pointers to the pop-up object and the string gadget
*       as its second and third arguments respectively.
*
*   SEE ALSO
*       MUIA_Popobject_ObjStrHook
*
******************************************************************************
*
*/

/****** Popobject.mui/MUIA_Popobject_Volatile ********************************
*
*   NAME
*       MUIA_Popobject_Volatile -- (V7) [ISG], BOOL
*
*   FUNCTION
*       Specifies whether the pop-up window should be hidden when the portion 
*       of the Popobject residing in the parent window is hidden. This would
*       normally be desirable if the Popobject is in a page group for 
*       example. If the pop-up window is on display when the Popobject is 
*       hidden, it will reappear as soon as the Popobject is reshown. 
*       Defaults to TRUE.
*
******************************************************************************
*
*/

/****** Popobject.mui/MUIA_Popobject_WindowHook ******************************
*
*   NAME
*       MUIA_Popobject_WindowHook -- (V9) [ISG], struct Hook *
*
*   FUNCTION
*       This hook is called just before the pop-up window is opened.
*
*       The hook receives pointers to the pop-up object and the window object
*       as its second and third arguments respectively.
*
*   SEE ALSO
*       MUIA_Popobject_ObjStrHook
*
*   INTERNALS
*       It is unclear whether this hook should be called every time the window
*       is opened or only the first time.
*
******************************************************************************
*
*/


AROS_UFH3(ULONG, Popobject_Open_Function,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(void **, msg, A1))
{
    AROS_USERFUNC_INIT

    struct Popobject_DATA *data = (struct Popobject_DATA *)hook->h_Data;
    Object *string = (Object *) msg[0];

    if (!data->wnd)
    {
        static struct TagItem light_tags[] =
        {
            {MUIA_Window_Borderless, TRUE},
            {MUIA_Window_CloseGadget, FALSE},
            {MUIA_Window_SizeGadget, FALSE},
            {MUIA_Window_DepthGadget, FALSE},
            {MUIA_Window_DragBar, FALSE},
            {TAG_DONE}
        };

        data->wnd = WindowObject,
            WindowContents, data->object,
            data->light ? TAG_MORE : TAG_IGNORE, (IPTR) light_tags, End;

        if (!data->wnd)
            return 0;

        DoMethod(_app(obj), OM_ADDMEMBER, (IPTR) data->wnd);

        DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR) obj, 2, MUIM_Popstring_Close, FALSE);
    }

    if (data->strobj_hook)
    {
        if (!(CallHookPkt(data->strobj_hook, data->object, string)))
            return 0;
    }

    if (data->window_hook)
    {
        CallHookPkt(data->window_hook, data->object, data->wnd);
    }

    SetAttrs(data->wnd,
        MUIA_Window_LeftEdge, _left(obj) + _window(obj)->LeftEdge,
        MUIA_Window_TopEdge, _bottom(obj) + 1 + _window(obj)->TopEdge,
        MUIA_Window_Width, _width(obj), MUIA_Window_Open, TRUE, TAG_DONE);
    data->popped = TRUE;

    return 1;

    AROS_USERFUNC_EXIT
}


AROS_UFH3(ULONG, Popobject_Close_Function,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(void **, msg, A1))
{
    AROS_USERFUNC_INIT

    struct Popobject_DATA *data = (struct Popobject_DATA *)hook->h_Data;
    Object *string = (Object *) msg[0];
    SIPTR suc = (SIPTR) msg[1];

    if (data->wnd)
    {
        set(data->wnd, MUIA_Window_Open, FALSE);
        data->popped = FALSE;

        if (data->objstr_hook && suc)
            CallHookPkt(data->objstr_hook, data->object, string);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

IPTR Popobject__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popobject_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->follow = TRUE;
    data->vol = TRUE;
    data->light = TRUE;

    data->open_hook.h_Entry = (HOOKFUNC) Popobject_Open_Function;
    data->open_hook.h_Data = data;
    data->close_hook.h_Entry = (HOOKFUNC) Popobject_Close_Function;
    data->close_hook.h_Data = data;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Popobject_Follow:
            data->follow = tag->ti_Data;
            break;

        case MUIA_Popobject_Light:
            data->light = tag->ti_Data;
            break;

        case MUIA_Popobject_Object:
            data->object = (Object *) tag->ti_Data;
            break;

        case MUIA_Popobject_ObjStrHook:
            data->objstr_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Popobject_StrObjHook:
            data->strobj_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Popobject_Volatile:
            data->vol = tag->ti_Data;
            break;

        case MUIA_Popobject_WindowHook:
            data->window_hook = (struct Hook *)tag->ti_Data;
            break;
        }
    }

    SetAttrs(obj,
        MUIA_Popstring_OpenHook, (IPTR) & data->open_hook,
        MUIA_Popstring_CloseHook, (IPTR) & data->close_hook,
        MUIA_Popstring_Toggle, TRUE,
        TAG_DONE);

    return (IPTR) obj;
}

IPTR Popobject__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    if (!data->wnd && data->object)
        MUI_DisposeObject(data->object);

    return DoSuperMethodA(cl, obj, msg);
}

IPTR Popobject__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);
    struct TagItem *tags;
    struct TagItem *tag;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Popobject_Follow:
            data->follow = tag->ti_Data;
            break;

        case MUIA_Popobject_ObjStrHook:
            data->objstr_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Popobject_StrObjHook:
            data->strobj_hook = (struct Hook *)tag->ti_Data;
            break;

        case MUIA_Popobject_Volatile:
            data->vol = tag->ti_Data;
            break;

        case MUIA_Popobject_WindowHook:
            data->window_hook = (struct Hook *)tag->ti_Data;
            break;
        }
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}


IPTR Popobject__MUIM_Get(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Popobject_Follow:
        *msg->opg_Storage = data->follow;
        return TRUE;

    case MUIA_Popobject_Light:
        *msg->opg_Storage = data->light;
        return TRUE;

    case MUIA_Popobject_Object:
        *msg->opg_Storage = (IPTR) data->object;
        return TRUE;

    case MUIA_Popobject_ObjStrHook:
        *msg->opg_Storage = (IPTR) data->objstr_hook;
        return TRUE;

    case MUIA_Popobject_StrObjHook:
        *msg->opg_Storage = (IPTR) data->strobj_hook;
        return TRUE;

    case MUIA_Popobject_Volatile:
        *msg->opg_Storage = data->vol;
        return TRUE;

    case MUIA_Popobject_WindowHook:
        *msg->opg_Storage = (IPTR) data->window_hook;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Popobject__MUIM_Show(struct IClass *cl, Object *obj,
    struct MUIP_Show *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    IPTR rc = DoSuperMethodA(cl, obj, (Msg) msg);

    /* If the pop-up window was shown when we were hidden, we reopen it, but
       keep it inactive to prevent objects in the parent window missing
       events (e.g. mouse up on a Listview) */
    if (data->popped)
    {
        if (!XGET(data->wnd, MUIA_Window_Open))
        {
            set(data->wnd, MUIA_Window_Activate, FALSE);
            set(data->wnd, MUIA_Window_Open, TRUE);
        }
    }
    if (!rc)
        return 0;
    return rc;
}

IPTR Popobject__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    /* Hide pop-up window too */
    if (data->popped)
        set(data->wnd, MUIA_Window_Open, FALSE);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR Popobject__MUIM_Draw(struct IClass *cl, Object *obj,
   struct MUIP_Draw *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);
    struct Window *popwin = NULL;
    struct Window *parentwin = _window(obj);

    DoSuperMethodA(cl, obj, (Msg) msg);

    get(data->wnd, MUIA_Window_Window, &popwin);

    if (data->follow && popwin && parentwin)
    {
        ChangeWindowBox(popwin, _left(obj) + parentwin->LeftEdge,
            _bottom(obj) + parentwin->TopEdge + 1,
            _width(obj), popwin->Height);
    }

    return 0;
}

BOOPSI_DISPATCHER(IPTR, Popobject_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Popobject__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Popobject__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Popobject__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Popobject__MUIM_Get(cl, obj, (struct opGet *)msg);
    case MUIM_Show:
        return Popobject__MUIM_Show(cl, obj, (struct MUIP_Show *)msg);
    case MUIM_Hide:
        return Popobject__MUIM_Hide(cl, obj, (struct MUIP_Hide *)msg);
    case MUIM_Draw:
        return Popobject__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Popobject_desc =
{
    MUIC_Popobject,
    MUIC_Popstring,
    sizeof(struct Popobject_DATA),
    (void *) Popobject_Dispatcher
};
