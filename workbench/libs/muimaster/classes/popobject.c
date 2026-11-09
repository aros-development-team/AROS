/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
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

/****** Popobject.mui/MUIC_Popobject ************
*
*   NAME
*       MUIC_Popobject
*
*   SUPERCLASS
*       Popstring.mui
*
*   DESCRIPTION
*       Place a caller-supplied object in a reusable popup window. Follow, Light
*       and Volatile default TRUE; conversion/window hooks default NULL.
*       Construction installs internal Popstring OpenHook/CloseHook and forces
*       Toggle TRUE after processing caller tags.
*       The popup object is owned by this arrangement. Before any window exists,
*       Dispose disposes the object directly. Once created, the window owns its
*       contents and is added to the application; this class does not remove or
*       dispose that window in its own Dispose. Keep its ownership and callback
*       lifetimes coordinated with application teardown.
*
*   METHODS
*       An open creates the window if needed, calls StrObjHook if supplied,
*       calls WindowHook, then requests a window below the Popobject at its
*       current width. A false StrObjHook result aborts this attempt but retains
*       an already-created window. WindowHook runs on every accepted open
*       attempt, not just initial creation. Its changes to left/top/width are
*       followed by the class's own settings. Actual Window_Open success is not
*       checked: popped becomes TRUE and the internal open hook returns one.
*       The queued close hook closes the retained window, clears popped and
*       calls ObjStrHook only for a nonzero result. Hide closes a popped window
*       only when Volatile; it preserves popped. Show reopens a remembered
*       closed window with Activate FALSE, without conversion/window hooks. Draw
*       implements Follow with ChangeWindowBox, preserving current height.
*
*   NOTES
*       The popup needs a live application and parent window while opening or
*       following. Close inherits Popstring's deferred execution and lifetime
*       requirements. A window can remain application-owned after Popobject
*       disposal with notifications still targeting it; disposal alone is not
*       callback detachment. Do not replace internal Popstring hooks when
*       relying on this class's lifecycle.
*       Light is initialize/get only here despite its header set annotation.
*       Object is initialize/get; the other local attributes support
*       initialize/set/get. No hooks or hook data are copied or freed.
*
*/
/****** Popobject.mui/MUIA_Popobject_Object ************
*
*   NAME
*       MUIA_Popobject_Object
*
*   FUNCTION
*       Supply or retrieve the popup contents object.
*
*   INPUTS
*       Initialize and get, Object pointer. No replacement setter; no default
*       popup object is created.
*
*   NOTES
*       Supply a valid object suitable as WindowContents. It is disposed
*       directly if no popup window exists at Popobject disposal; otherwise the
*       application-owned popup window owns it. Get returns a borrowed pointer,
*       not independent ownership. Keep the object stable for conversion hooks
*       and window lifetime.
*
*/
/****** Popobject.mui/MUIA_Popobject_Light ************
*
*   NAME
*       MUIA_Popobject_Light
*
*   FUNCTION
*       Choose borderless popup construction.
*
*   INPUTS
*       Initialize and get, BOOL; default TRUE. No local setter despite the
*       header annotation.
*
*   NOTES
*       TRUE creates a borderless window with close, size, depth and drag
*       gadgets disabled. FALSE omits these override tags and uses Window
*       defaults. It affects initial window construction, not an existing
*       window. WindowHook may configure window attributes afterward.
*
*/
/****** Popobject.mui/MUIA_Popobject_Follow ************
*
*   NAME
*       MUIA_Popobject_Follow
*
*   FUNCTION
*       Follow the parent position and Popobject width during Draw.
*
*   INPUTS
*       Initialize, set and get, BOOL; default TRUE.
*
*   NOTES
*       After superclass Draw, a live popup and parent window are repositioned
*       with ChangeWindowBox to Popobject left/bottom plus parent window
*       position, at Popobject width and current popup height. The setter only
*       stores the flag. Initial Open positions the window regardless of Follow.
*       This is draw-driven behavior, not a separately registered movement
*       listener.
*
*/
/****** Popobject.mui/MUIA_Popobject_Volatile ************
*
*   NAME
*       MUIA_Popobject_Volatile
*
*   FUNCTION
*       Hide a popped window when this object is hidden.
*
*   INPUTS
*       Initialize, set and get, BOOL; default TRUE.
*
*   NOTES
*       Hide closes the window when popped and Volatile but retains popped
*       state. Show reopens a remembered closed popup with Activate FALSE and no
*       conversion/window hooks, even before returning a failed superclass Show
*       result. A real Close clears popped and prevents this reopening. Set only
*       changes the stored flag; it does not immediately open or close a window.
*
*/
/****** Popobject.mui/MUIA_Popobject_StrObjHook ************
*
*   NAME
*       MUIA_Popobject_StrObjHook
*
*   FUNCTION
*       Initialize popup contents from the string before opening.
*
*   INPUTS
*       Initialize, set and get, borrowed pointer to struct Hook; default NULL.
*
*   NOTES
*       Called as CallHookPkt(hook, popupObject, stringObject). Return nonzero
*       to continue, zero to reject opening. It runs after lazy window creation,
*       before WindowHook and geometry/open settings. Rejection retains the
*       window and its application membership. Keep hook, data and passed
*       objects live through use; Get returns the borrowed hook pointer.
*
*/
/****** Popobject.mui/MUIA_Popobject_ObjStrHook ************
*
*   NAME
*       MUIA_Popobject_ObjStrHook
*
*   FUNCTION
*       Copy accepted popup contents to the string.
*
*   INPUTS
*       Initialize, set and get, borrowed pointer to struct Hook; default NULL.
*
*   NOTES
*       Called as CallHookPkt(hook, popupObject, stringObject) from the deferred
*       close hook after closing the window, only with nonzero Close result.
*       Return value is ignored. No automatic string update or Acknowledge
*       occurs; the hook implements any transfer. Keep hook, data and passed
*       objects live through use; Get returns the borrowed hook pointer.
*
*/
/****** Popobject.mui/MUIA_Popobject_WindowHook ************
*
*   NAME
*       MUIA_Popobject_WindowHook
*
*   FUNCTION
*       Configure the popup window before an open attempt.
*
*   INPUTS
*       Initialize, set and get, borrowed pointer to struct Hook; default NULL.
*
*   NOTES
*       Called as CallHookPkt(hook, popupObject, windowObject) after a
*       successful StrObjHook, on every normal open attempt. Return value is
*       ignored. The class then sets left, top, width and Open. Show-based
*       reappearance does not invoke this hook. Keep hook, data and passed
*       objects live through use; Get returns the borrowed hook pointer.
*
*/

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









/*
 * Use borrowed instance hook data and message word zero as String. Lazily
 * create application-owned popup, run conversion/window hooks, request
 * geometry/open and mark popped. Return zero on creation or conversion
 * rejection, otherwise one without checking actual window open. Requires
 * live parent/application and stable popup contents.
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


/*
 * Use message words String and result; close retained window and clear
 * popped. For nonzero result invoke borrowed ObjStrHook, ignoring its
 * return. Return zero. Runs after queuing, so instance, contents and
 * callback resources must remain live.
 */
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

/*
 * Construct Popstring, store popup object and borrowed hooks, set defaults
 * TRUE and install embedded Open/Close hooks with Toggle TRUE. Return
 * object or zero on superclass failure. Ownership of contents follows lazy
 * window creation and application lifecycle.
 */
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

/*
 * Dispose contents directly only if no popup window exists, then return
 * Popstring disposal result. Existing window remains application-owned;
 * notifications and queued callbacks must no longer target this instance.
 * No local window detachment or queue cancellation.
 */
IPTR Popobject__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    if (!data->wnd && data->object)
        MUI_DisposeObject(data->object);

    return DoSuperMethodA(cl, obj, msg);
}

/*
 * Store Follow, Volatile and borrowed conversion/window hooks; return
 * Popstring result. Light/Object have no local setter. Does not immediately
 * reposition, hide or recreate a window; callback lifetimes remain caller-
 * owned.
 */
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


/*
 * Handle OM_GET despite function name: write local flags or borrowed
 * object/hook pointers and return TRUE; delegate other attributes to
 * Popstring. Requires valid caller storage and grants no ownership.
 */
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

/*
 * Call Popstring Show, reopen remembered closed popup without activation or
 * hooks, then return superclass result even if it failed. Requires retained
 * live popup; no ownership transfer.
 */
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

/*
 * Hide popped window only when Volatile, retaining popped state for Show,
 * then return Popstring result. Does not execute conversion hooks or clear
 * logical Popstring open state; requires live window.
 */
IPTR Popobject__MUIM_Hide(struct IClass *cl, Object *obj,
    struct MUIP_Hide *msg)
{
    struct Popobject_DATA *data = INST_DATA(cl, obj);

    /* Hide pop-up window too */
    if (data->popped && data->vol)
        set(data->wnd, MUIA_Window_Open, FALSE);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Call Popstring Draw, query popup native window and apply Follow geometry
 * when both windows exist, preserving height. Return zero. Requires valid
 * parent geometry and retained popup; no explicit local flag gate or
 * allocation.
 */
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

/*
 * Dispatch local overrides and return their results; delegate other
 * messages to superclass. Requires ABI-correct message and live object
 * except construction. Deferred callbacks and resource ownership follow the
 * class lifecycle contract.
 */
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
