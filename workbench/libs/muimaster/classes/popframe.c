/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

/*  #define MYDEBUG 1 */
#include "debug.h"
#include "locale.h"
#include "mui.h"
#include "muimaster_intern.h"
#include "popframe_private.h"
#include "support.h"
#include "support_classes.h"

/****** Popframe.mui/MUIC_Popframe ************
*
*   NAME
*       MUIC_Popframe
*
*   SUPERCLASS
*       Framedisplay.mui
*
*   DESCRIPTION
*       Display a draggable button preview and open a Frameadjust editor when
*       Pressed becomes FALSE. Constructor defaults are ButtonFrame, inner
*       spacing four, button background, RelVerify input mode and Draggable
*       TRUE, followed by caller tags. Construction returns NULL on superclass
*       failure. The preview specification is inherited from Framedisplay.
*       MUIA_Window_Title is read only at construction as a borrowed popup-title
*       pointer, default NULL. It must remain valid through future popup
*       creation; no local title setter/getter is provided.
*
*   METHODS
*       Opening lazily creates an activated subwindow below the object, adds it
*       to the application and initializes the editor from the preview
*       specification. An existing window is reused without resampling the
*       preview. Internal Open returns one even on construction/open failure;
*       failed opening of a created window invokes cancellation.
*       OK copies the editor specification into the preview before removing and
*       disposing the window. Cancel and window close requests discard edits.
*       Notifications enqueue the private close method through
*       Application_PushMethod. Hide only delegates: its local window-closing
*       code is compiled out. DisconnectParent cancels an existing popup before
*       superclass disconnection.
*
*   NOTES
*       Use a live application and parent window. Keep this object alive through
*       queued closes. The private Close assumes an existing popup, ignores
*       getter/setter status and has no repeated-call guard or queue
*       cancellation. Dispose directly disposes a remaining popup without first
*       removing its application membership; normal disconnection should
*       close/remove it first. Do not treat Hide or Dispose as a queue drain.
*       The private window methods are not public API.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Construct Framedisplay with button defaults, borrow Window_Title and
 * register release notification for private MUIM_Popframe_OpenWindow.
 * Return object or zero on superclass failure. Title must survive future
 * opens; notification registration status is ignored.
 */
IPTR Popframe__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Popframe_DATA *data;
    struct TagItem *tag, *tags;

    obj = (Object *)DoSuperNewTags(
              cl, obj, NULL, ButtonFrame, InnerSpacing(4, 4), MUIA_Background,
              MUII_ButtonBack, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_Draggable,
              TRUE, TAG_MORE, (IPTR)msg->ops_AttrList);

    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->wintitle = NULL;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));) {
        switch (tag->ti_Tag) {
        case MUIA_Window_Title:
            data->wintitle = (CONST_STRPTR)tag->ti_Data;
            break;
        }
    }

    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1,
             MUIM_Popframe_OpenWindow);

    return (IPTR)obj;
}

/*
 * Dispose remaining popup directly and clear wnd, then return superclass
 * result. Requires queued callbacks finished and proper prior
 * disconnection: this fallback does not remove application membership.
 * Editor pointer is not separately cleared; do not retain child pointers.
 */
IPTR Popframe__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Popframe_DATA *data = INST_DATA(cl, obj);

    if (data->wnd) {
        D(bug("Popframe_Dispose(%p) : MUI_DisposeObject(%p)\n", obj, data->wnd));
        MUI_DisposeObject(data->wnd);
        data->wnd = NULL;
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*
 * Delegate to superclass and return its result. Local popup cleanup is
 * inside #if 0 and does not execute. A popup can remain open while this
 * preview is hidden; callback lifetimes remain required.
 */
IPTR Popframe__MUIM_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
#if 0
    struct Popframe_DATA *data = INST_DATA(cl, obj);
    if (data->wnd) {
        D(bug("Popframe_Hide(%p) : closing window %p\n", obj, data->wnd));
        set(data->wnd, MUIA_Window_Open, FALSE);
        D(bug("Popframe_Hide(%p) : app REMMEMBER win (%p)\n", obj,
              data->wnd));
        DoMethod(_app(obj), OM_REMMEMBER, (IPTR) data->wnd);
        D(bug("Popframe_Hide(%p) : MUI_DisposeObject(%p)\n", obj,
              data->wnd));
        MUI_DisposeObject(data->wnd);
        data->wnd = NULL;
    }
#endif
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/*
 * Private MUIM_Popframe_OpenWindow lazily constructs an application
 * subwindow from current preview, registers queued Close callbacks and
 * opens it. Return one even on failure; cancel if created window fails to
 * open. Requires live parent/application/title and valid specification;
 * existing window is reused.
 */
IPTR Popframe__MUIM_Popframe_OpenWindow(struct IClass *cl, Object *obj,
                                        Msg msg)
{
    struct Popframe_DATA *data = INST_DATA(cl, obj);

    if (!data->wnd) {
        Object *ok_button, *cancel_button;
        struct MUI_Frame_Spec *frame_spec = NULL;
        ULONG x = 0, y = 0;

        get(_win(obj), MUIA_Window_LeftEdge, &x);
        get(_win(obj), MUIA_Window_TopEdge, &y);

        get(obj, MUIA_Framedisplay_Spec, &frame_spec);

        // clang-format off
        data->wnd = (Object *)WindowObject,
              MUIA_Window_Title, (IPTR)data->wintitle,
              MUIA_Window_Activate, TRUE,
              MUIA_Window_IsSubWindow, TRUE,
              MUIA_Window_LeftEdge, _left(obj) + x,
              MUIA_Window_TopEdge, _bottom(obj) + y + 1,
              WindowContents, (IPTR)VGroup,
              Child, (IPTR)(data->frameadjust = MUI_NewObject(MUIC_Frameadjust,
                                                MUIA_CycleChain, 1,
                                                MUIA_Frameadjust_Spec, (IPTR)frame_spec,
                                                TAG_DONE)),
              Child, (IPTR)HGroup,
              MUIA_Group_SameWidth, TRUE,
              Child, (IPTR)(ok_button = MUI_MakeObject(MUIO_Button, (IPTR)_(MSG_OK))),
              Child, (IPTR)HVSpace,
              Child, (IPTR)HVSpace,
              Child, (IPTR)(cancel_button = MUI_MakeObject(MUIO_Button, (IPTR)_(MSG_CANCEL))),
              End,
              End,
              End;
        // clang-format on

        if (data->wnd) {
            set(ok_button, MUIA_CycleChain, 1);
            set(cancel_button, MUIA_CycleChain, 1);

            DoMethod(_app(obj), OM_ADDMEMBER, (IPTR)data->wnd);

            DoMethod(ok_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj), 5,
                     MUIM_Application_PushMethod, (IPTR)obj, 2,
                     MUIM_Popframe_CloseWindow, TRUE);
            DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)_app(obj),
                     5, MUIM_Application_PushMethod, (IPTR)obj, 2,
                     MUIM_Popframe_CloseWindow, FALSE);
            DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                     (IPTR)_app(obj), 5, MUIM_Application_PushMethod, (IPTR)obj, 2,
                     MUIM_Popframe_CloseWindow, FALSE);
        }
    }

    if (data->wnd) {
        ULONG opened = FALSE;

        set(data->wnd, MUIA_Window_Open, TRUE);
        get(data->wnd, MUIA_Window_Open, &opened);
        if (!opened) {
            DoMethod(obj, MUIM_Popframe_CloseWindow, FALSE);
        }
    }

    return 1;
}

/*
 * Private MUIM_Popframe_CloseWindow closes popup, copies editor Spec to
 * preview only for nonzero ok, removes/disposes window and clears
 * window/editor pointers. Return one. Requires existing live
 * popup/application; ignores transfer status and does not guard stale
 * queued calls.
 */
IPTR Popframe__MUIM_Popframe_CloseWindow(
    struct IClass *cl, Object *obj, struct MUIP_Popframe_CloseWindow *msg)
{
    struct Popframe_DATA *data = INST_DATA(cl, obj);
    int ok = msg->ok;

    set(data->wnd, MUIA_Window_Open, FALSE);

    if (ok) {
        STRPTR spec = NULL;
        get(data->frameadjust, MUIA_Frameadjust_Spec, &spec);
        /*          D(bug("popframe: got %s\n", spec)); */
        set(obj, MUIA_Framedisplay_Spec, (IPTR)spec);
    }

    DoMethod(_app(obj), OM_REMMEMBER, (IPTR)data->wnd);
    MUI_DisposeObject(data->wnd);
    data->wnd = NULL;
    data->frameadjust = NULL;
    return 1;
}

/*
 * Cancel an existing popup while application connection is valid, then
 * return superclass disconnect result. Discards edits and removes/disposes
 * the window; does not cancel already queued close callbacks.
 */
IPTR Popframe__MUIM_DisconnectParent(struct IClass *cl, Object *obj,
                                     struct MUIP_DisconnectParent *msg)
{
    struct Popframe_DATA *data = INST_DATA(cl, obj);

    if (data->wnd)
        DoMethod(obj, MUIM_Popframe_CloseWindow, FALSE);
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

#if ZUNE_BUILTIN_POPFRAME
/*
 * Dispatch local overrides and return their results, delegating other
 * messages to superclass. Requires ABI-correct message and live object
 * except construction. Resources and deferred callbacks follow the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Popframe_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID) {
    case OM_NEW:
        return Popframe__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Popframe__OM_DISPOSE(cl, obj, msg);
    case MUIM_Hide:
        return Popframe__MUIM_Hide(cl, obj, (APTR)msg);
    case MUIM_Popframe_OpenWindow:
        return Popframe__MUIM_Popframe_OpenWindow(cl, obj, (APTR)msg);
    case MUIM_Popframe_CloseWindow:
        return Popframe__MUIM_Popframe_CloseWindow(cl, obj, (APTR)msg);
    case MUIM_DisconnectParent:
        return Popframe__MUIM_DisconnectParent(cl, obj, (APTR)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Popframe_desc = {
    MUIC_Popframe, MUIC_Framedisplay, sizeof(struct Popframe_DATA),
    (void *)Popframe_Dispatcher
};
#endif /* ZUNE_BUILTIN_POPFRAME */
