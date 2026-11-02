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

/*  #define MYDEBUG 0 */
#include "debug.h"

/****** Popstring.mui/MUIC_Popstring ************
*
*   NAME
*       MUIC_Popstring
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Combine a caller-supplied button and optional string object in a
*       horizontal group with zero spacing. Successful construction places these
*       objects in the Group child hierarchy. No default button or string is
*       created. Supply a valid button; construction passes it as Child without
*       a NULL guard. The optional string pointer may be NULL only if the hooks
*       can handle it.
*       The button's Pressed FALSE notification invokes Open. OpenHook and
*       CloseHook are borrowed, default NULL; Toggle defaults FALSE. This base
*       class supplies hook dispatch and logical open state, not a popup window
*       or requester.
*
*   METHODS
*       Setup delegates to Group and installs a raw-key event handler in the
*       containing window; Cleanup removes it before delegating. MUIKEY_POPUP
*       invokes Open and consumes the event regardless of opening success. Other
*       events return zero.
*       The open hook is synchronous. Close queues a CallHook through the
*       application and immediately resets logical open state and enables the
*       button. The queued call contains the hook pointer, string pointer and
*       result value. There is no local cancellation, queue-success check or
*       wait. Cleanup does not call Close or wait for queued hooks.
*
*   NOTES
*       Keep the application, Popstring, children, hook structures and
*       referenced hook data alive through queued execution. Do not assume a
*       zero Open/Close result signals failure or completion. Avoid reopening
*       while a previous close callback remains queued: logical state has
*       already changed, but the old callback can still affect popup resources.
*       Hook replacement does not change a hook pointer already copied into a
*       queued call.
*       Only String has a local getter. The header's getter annotations for
*       Button, OpenHook, CloseHook and Toggle do not correspond to local getter
*       implementations.
*
*/
/****** Popstring.mui/MUIA_Popstring_Button ************
*
*   NAME
*       MUIA_Popstring_Button
*
*   FUNCTION
*       Supply the button that requests opening.
*
*   INPUTS
*       Initialize only in this implementation, Object pointer. No default
*       object and no local getter despite the header annotation.
*
*   NOTES
*       The button becomes a Group child on successful construction; do not
*       dispose it independently. Its Pressed FALSE notification opens the
*       popup. A successful non-toggle open disables it; a close with a non-NULL
*       close hook enables it immediately. Button replacement is not
*       implemented.
*
*/
/****** Popstring.mui/MUIA_Popstring_String ************
*
*   NAME
*       MUIA_Popstring_String
*
*   FUNCTION
*       Supply or retrieve the string-side child.
*
*   INPUTS
*       Initialize and get, Object pointer; default NULL. No local setter.
*
*   NOTES
*       A non-NULL object becomes a Group child. Get returns a borrowed pointer.
*       Hooks receive this pointer unchanged and must accept NULL if no string
*       was supplied. Subclasses that interpret String_Contents require a
*       compatible live string object. The base class neither copies text nor
*       updates string contents.
*
*/
/****** Popstring.mui/MUIA_Popstring_OpenHook ************
*
*   NAME
*       MUIA_Popstring_OpenHook
*
*   FUNCTION
*       Supply the synchronous opening hook.
*
*   INPUTS
*       Initialize and set, pointer to struct Hook; default NULL. No local
*       getter despite the header annotation.
*
*   NOTES
*       The hook is borrowed. MUIM_CallHook passes the Popstring object as hook
*       object and a pointer to argument words as message. The first word is the
*       String object. A nonzero hook return marks the popup logically open;
*       zero leaves it closed. With no hook Open does nothing.
*
*/
/****** Popstring.mui/MUIA_Popstring_CloseHook ************
*
*   NAME
*       MUIA_Popstring_CloseHook
*
*   FUNCTION
*       Supply the queued closing hook.
*
*   INPUTS
*       Initialize and set, pointer to struct Hook; default NULL. No local
*       getter despite the header annotation.
*
*   NOTES
*       The hook is borrowed. MUIM_CallHook passes the Popstring object as hook
*       object and a pointer to argument words as message. The first word is the
*       String object and the second is the Close result. Its return is ignored.
*       Close queues the hook only while logically open; with no hook it does
*       not reset state or enable the button. Retain hook and data until queued
*       execution completes.
*
*/
/****** Popstring.mui/MUIA_Popstring_Toggle ************
*
*   NAME
*       MUIA_Popstring_Toggle
*
*   FUNCTION
*       Control repeated Open requests while logically open.
*
*   INPUTS
*       Initialize and set, BOOL; default FALSE. No local getter despite the
*       header annotation.
*
*   NOTES
*       With TRUE and a non-NULL OpenHook, an Open request while already open
*       invokes Close with FALSE. With FALSE, an already-open request does
*       nothing and successful opening disables the button. Changing Toggle does
*       not immediately synchronize button-disabled state. A close requires
*       CloseHook to reset logical state.
*
*/
/****** Popstring.mui/MUIM_Popstring_Open ************
*
*   NAME
*       MUIM_Popstring_Open
*
*   SYNOPSIS
*       IPTR rc = DoMethod(obj, MUIM_Popstring_Open);
*
*   FUNCTION
*       Call the open hook or request a toggle-close.
*
*   INPUTS
*       No additional arguments.
*
*   RESULT
*       Zero in all cases.
*
*   NOTES
*       A nonzero synchronous OpenHook result sets logical open state and
*       disables the button when Toggle is FALSE. With no hook nothing happens.
*       If already open, Toggle TRUE requests Close(FALSE), otherwise nothing
*       happens. The public result does not expose the hook result.
*
*/
/****** Popstring.mui/MUIM_Popstring_Close ************
*
*   NAME
*       MUIM_Popstring_Close
*
*   SYNOPSIS
*       IPTR rc = DoMethod(obj, MUIM_Popstring_Close, result);
*
*   FUNCTION
*       Queue the close hook for a logically open popup.
*
*   INPUTS
*       result - LONG passed to the close hook; subclasses commonly interpret
*       nonzero as accept and zero as cancel.
*
*   RESULT
*       Zero in all cases.
*
*   NOTES
*       With CloseHook and logical open state, enqueue CallHook, clear that
*       state and enable the button immediately. The hook runs later through the
*       application; allocation/enqueue failure is not checked. With no hook or
*       already closed, do nothing. This is not an ASL abort or a synchronous
*       completion barrier.
*
*/

extern struct Library *MUIMasterBase;

struct Popstring_DATA
{
    struct Hook *close_hook;
    struct Hook *open_hook;
    Object *string;
    Object *button;
    int open;
    int toggle;
    struct MUI_EventHandlerNode ehn;
};


/*
 * Construct a horizontal Group owning supplied children, store borrowed
 * hooks/flags and register button release notification. Return object or
 * zero on superclass failure. Requires a valid button; optional string must
 * suit hooks. No default children or local getter support beyond String.
 */
IPTR Popstring__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popstring_DATA *data;
    struct TagItem *tag, *tags;
    Object *string, *button;

    button =
        (Object *) GetTagData(MUIA_Popstring_Button, 0, msg->ops_AttrList);
    string =
        (Object *) GetTagData(MUIA_Popstring_String, 0, msg->ops_AttrList);

    obj = (Object *) DoSuperNewTags(cl, obj, 0,
        MUIA_Group_Horiz, TRUE,
        MUIA_Group_Spacing, 0,
        (string ? Child : TAG_IGNORE), (IPTR) string,
        Child, (IPTR) button,
        TAG_MORE, (IPTR) msg->ops_AttrList);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->button = button;
    data->string = string;

    data->ehn.ehn_Events = IDCMP_RAWKEY;
    data->ehn.ehn_Priority = 0;
    data->ehn.ehn_Flags = 0;
    data->ehn.ehn_Object = obj;
    data->ehn.ehn_Class = cl;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Popstring_Toggle:
            data->toggle = tag->ti_Data;
            break;
        case MUIA_Popstring_CloseHook:
            data->close_hook = (struct Hook *)tag->ti_Data;
            break;
        case MUIA_Popstring_OpenHook:
            data->open_hook = (struct Hook *)tag->ti_Data;
            break;
        }
    }

    DoMethod(button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) obj, 1,
        MUIM_Popstring_Open);

    return (IPTR) obj;
}

/*
 * Replace borrowed hooks and Toggle in tag order, then return Group setter
 * result. Does not cancel queued callbacks, change existing queue payloads
 * or immediately synchronize button state. Caller maintains hook/data
 * lifetimes.
 */
IPTR Popstring__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TagItem *tags, *tag;
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Popstring_Toggle:
            data->toggle = tag->ti_Data;
            break;
        case MUIA_Popstring_CloseHook:
            data->close_hook = (struct Hook *)tag->ti_Data;
            break;
        case MUIA_Popstring_OpenHook:
            data->open_hook = (struct Hook *)tag->ti_Data;
            break;
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#define STORE *(msg->opg_Storage)
/*
 * Return borrowed String object through valid storage with one; delegate
 * all other queries to Group. Header getter annotations for
 * Button/hooks/Toggle are not implemented locally. No ownership transfer.
 */
IPTR Popstring__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Popstring_String:
        STORE = (IPTR) data->string;
        return 1;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}
#undef STORE

/*
 * Call Group setup and return FALSE on failure; otherwise register instance
 * raw-key handler in the containing window and return TRUE. Requires live
 * window and matched Cleanup before disposal.
 */
IPTR Popstring__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    if (!(DoSuperMethodA(cl, obj, (Msg) msg)))
        return FALSE;

    DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR) & data->ehn);

    return TRUE;
}

/*
 * Remove instance event handler from the live containing window and return
 * Group cleanup result. Does not close a popup or drain queued hooks; their
 * lifetimes remain caller responsibilities.
 */
IPTR Popstring__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);

    DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR) & data->ehn);

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * On MUIKEY_POPUP invoke Open and return Eat regardless of hook result;
 * otherwise return zero. Requires live object and correctly formed event
 * message. No ownership transfer.
 */
IPTR Popstring__MUIM_HandleEvent(struct IClass *cl, Object *obj,
    struct MUIP_HandleEvent *msg)
{
    if (msg->muikey == MUIKEY_POPUP)
    {
        D(bug("Popstring__MUIM_HandleEvent %p got MUIKEY_POPUP\n", obj));
        DoMethod(obj, MUIM_Popstring_Open);
        return MUI_EventHandlerRC_Eat;
    }
    return 0;
}

/*
 * If close hook and logical open state exist, enqueue CallHook with
 * borrowed hook/string and result, reset state and enable button. Always
 * return zero, ignoring enqueue failure. Requires live application and
 * retained callback resources until deferred execution.
 */
IPTR Popstring__MUIM_Popstring_Close(struct IClass *cl, Object *obj,
    struct MUIP_Popstring_Close *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);
    if (data->close_hook && data->open)
    {
        DoMethod(_app(obj), MUIM_Application_PushMethod, (IPTR) obj, 4,
            MUIM_CallHook, (IPTR) data->close_hook, (IPTR) data->string,
            msg->result);
        data->open = 0;
        set(data->button, MUIA_Disabled, FALSE);
    }
    return 0;
}

/*
 * Invoke synchronous borrowed open hook when closed, or request toggle
 * cancellation when already open. Always return zero. Hook arguments and
 * logical/button effects follow public Open contract; queued prior closes
 * remain outstanding.
 */
IPTR Popstring__MUIM_Popstring_Open(struct IClass *cl, Object *obj,
    struct MUIP_Popstring_Open *msg)
{
    struct Popstring_DATA *data = INST_DATA(cl, obj);
    if (data->open_hook)
    {
        if (data->open && data->toggle)
        {
            DoMethod(obj, MUIM_Popstring_Close, FALSE);
            return 0;
        }

        if (!data->open)
        {
            if (DoMethod(obj, MUIM_CallHook, (IPTR) data->open_hook,
                    (IPTR) data->string))
            {
                /* Opening the popup window was successful */
                data->open = 1;
                if (!data->toggle)
                    set(data->button, MUIA_Disabled, TRUE);
            }
        }
    }
    return 0;
}


/*
 * Dispatch local overrides and return their results; delegate other
 * messages to superclass. Requires ABI-correct message and live object
 * except construction. Deferred callbacks and resource ownership follow the
 * class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Popstring_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Popstring__OM_NEW(cl, obj, (APTR) msg);
    case OM_SET:
        return Popstring__OM_SET(cl, obj, (APTR) msg);
    case OM_GET:
        return Popstring__OM_GET(cl, obj, (APTR) msg);
    case MUIM_Setup:
        return Popstring__MUIM_Setup(cl, obj, (APTR) msg);
    case MUIM_Cleanup:
        return Popstring__MUIM_Cleanup(cl, obj, (APTR) msg);
    case MUIM_HandleEvent:
        return Popstring__MUIM_HandleEvent(cl, obj, (APTR) msg);
    case MUIM_Popstring_Close:
        return Popstring__MUIM_Popstring_Close(cl, obj, (APTR) msg);
    case MUIM_Popstring_Open:
        return Popstring__MUIM_Popstring_Open(cl, obj, (APTR) msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Popstring_desc =
{
    MUIC_Popstring,
    MUIC_Group,
    sizeof(struct Popstring_DATA),
    (void *) Popstring_Dispatcher
};
