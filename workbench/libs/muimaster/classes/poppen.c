/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "poppen_private.h"
#include "locale.h"

/****** Poppen.mui/MUIC_Poppen ************
*
*   NAME
*       MUIC_Poppen
*
*   SUPERCLASS
*       Pendisplay.mui
*
*   DESCRIPTION
*       Display a pen as a draggable button that opens a Penadjust window when
*       Pressed becomes FALSE. It inherits Pendisplay color attributes and their
*       ownership, reference and notification behavior. Constructor defaults
*       include ButtonFrame, inner spacing four, button background, RelVerify
*       input mode and Draggable TRUE, followed by caller tags.
*       MUIA_Window_Title is consumed at construction as a borrowed popup-title
*       pointer, default NULL. This is not a local set/get attribute: changing
*       that tag later does not update the saved title. Keep the string valid
*       for every popup creation. Pen state remains in the inherited Pendisplay
*       specification.
*
*   METHODS
*       Opening lazily creates a window containing Penadjust and localized
*       OK/Cancel buttons, adds it to the application and opens it. An existing
*       popup is reused without resampling the current pen. Creation/open
*       failure is not reported by the internal open method, which returns one;
*       failed opening closes the created window with cancellation.
*       OK reads the editor specification, copies it into Pendisplay and
*       disposes the popup. Cancel or the window close request discards edits.
*       Button and close-request notifications enqueue the internal close method
*       through Application_PushMethod. Hide and DisconnectParent close/dispose
*       an existing popup without committing edits before delegating to the
*       superclass.
*
*   NOTES
*       Use a live application connection and keep this object alive until its
*       queued close calls have run. The private close method assumes an
*       existing popup and does not guard repeated or stale calls. Hide clears
*       wnd but leaves the penadjust pointer stale; no independent queue
*       cancellation occurs here. Do not invoke internal window methods as
*       public API. There is no local Dispose override to replace normal
*       Hide/DisconnectParent ordering.
*       Opening obtains the inherited Spec without checking getter success
*       before constructing Penadjust. Any Reference must therefore be live and
*       provide a valid full specification. With Reference attached, OK changes
*       hidden local Pendisplay state; it does not change the referenced object
*       or override redirected getters.
*
*/

extern struct Library *MUIMasterBase;


/*
 * Construct Pendisplay with button defaults and caller tags, borrow
 * constructor Window_Title and register release notification for private
 * MUIM_Poppen_OpenWindow. Return object or zero on superclass failure.
 * Title must remain valid for future popup creation; registration failure
 * is ignored.
 */
IPTR Poppen__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Poppen_DATA *data;
    struct TagItem *tag, *tags;
    //Object *image;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        ButtonFrame,
        InnerSpacing(4, 4),
        MUIA_Background, MUII_ButtonBack,
        MUIA_InputMode, MUIV_InputMode_RelVerify,
        MUIA_Draggable, TRUE, TAG_MORE, (IPTR) msg->ops_AttrList);

    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);
    data->wintitle = NULL;

    /* parse initial taglist */
    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Window_Title:
            data->wintitle = (CONST_STRPTR) tag->ti_Data;
            break;

        }
    }

    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR) obj, 1,
        MUIM_Poppen_OpenWindow);

    return (IPTR) obj;
}

/*
 * Close, remove and dispose an existing popup without committing edits,
 * clear wnd and return Pendisplay Hide result. Requires live application
 * connection. Leaves penadjust stale and does not cancel queued close
 * calls; retain object until queued work is resolved.
 */
IPTR Poppen__MUIM_Hide(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);

    if (data->wnd)
    {
        set(data->wnd, MUIA_Window_Open, FALSE);
        DoMethod(_app(obj), OM_REMMEMBER, (IPTR) data->wnd);
        MUI_DisposeObject(data->wnd);
        data->wnd = NULL;
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Private MUIM_Poppen_OpenWindow: lazily create application-owned editor
 * window from inherited Spec, install queued close notifications and open
 * it. Return one even on failure; cancel a created window if open fails.
 * Requires live application, title and full Spec. Existing popup is reused
 * without refresh.
 */
IPTR Poppen__MUIM_Poppen_OpenWindow(struct IClass *cl, Object *obj,
    Msg msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);

    if (!data->wnd)
    {
        Object *ok_button, *cancel_button;
        char *penspec = NULL;

        get(obj, MUIA_Pendisplay_Spec, &penspec);

        data->wnd = (Object *)(WindowObject,
            MUIA_Window_Title, (IPTR)data->wintitle,
            MUIA_Window_Activate, TRUE,
                WindowContents, (IPTR)VGroup,
                Child, (IPTR)(data->penadjust = (Object *)PenadjustObject,
                    MUIA_CycleChain, 1,
                    MUIA_Penadjust_Spec, (IPTR)penspec,
                    End),
                Child, (IPTR)HGroup,
                    Child, (IPTR)(ok_button =
                        MUI_MakeObject(MUIO_Button,(IPTR)_(MSG_OK))),
                    Child, (IPTR)(cancel_button =
                        MUI_MakeObject(MUIO_Button,(IPTR)_(MSG_CANCEL))),
                    End,
                End,
            End);

        if (data->wnd)
        {
            set(ok_button, MUIA_CycleChain, 1);
            set(cancel_button, MUIA_CycleChain, 1);

            DoMethod(_app(obj), OM_ADDMEMBER, (IPTR) data->wnd);

            DoMethod(ok_button, MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR) _app(obj), 5, MUIM_Application_PushMethod,
                (IPTR) obj, 2, MUIM_Poppen_CloseWindow, TRUE);
            DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR) _app(obj), 5, MUIM_Application_PushMethod,
                (IPTR) obj, 2, MUIM_Poppen_CloseWindow, FALSE);
            DoMethod(data->wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                (IPTR) _app(obj), 5, MUIM_Application_PushMethod,
                (IPTR) obj, 2, MUIM_Poppen_CloseWindow, FALSE);
        }
    }

    if (data->wnd)
    {
        IPTR opened = FALSE;

        set(data->wnd, MUIA_Window_Open, TRUE);
        get(data->wnd, MUIA_Window_Open, &opened);

        if (!opened)
        {
            DoMethod(obj, MUIM_Poppen_CloseWindow, FALSE);
        }
    }

    return 1;
}

/*
 * Private MUIM_Poppen_CloseWindow: close current popup; nonzero ok copies
 * editor Spec to Pendisplay, zero discards edits. Remove/dispose window,
 * clear both pointers and return one. Requires an existing live popup and
 * application; no NULL/repeated-call guard or queue cancellation.
 */
IPTR Poppen__MUIM_Poppen_CloseWindow(struct IClass *cl, Object *obj,
    struct MUIP_Poppen_CloseWindow *msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);
    int ok = msg->ok;

    set(data->wnd, MUIA_Window_Open, FALSE);

    if (ok)
    {
        char *spec = NULL;

        get(data->penadjust, MUIA_Penadjust_Spec, &spec);

        set(obj, MUIA_Pendisplay_Spec, (IPTR) spec);
    }

    DoMethod(_app(obj), OM_REMMEMBER, (IPTR) data->wnd);
    MUI_DisposeObject(data->wnd);
    data->wnd = NULL;
    data->penadjust = NULL;
    return 1;
}

/*
 * Cancel a live popup through private CloseWindow before delegating and
 * returning Pendisplay disconnect result. Requires application still
 * connected for removal. No edit commit; outstanding queued calls are not
 * canceled.
 */
IPTR Poppen__MUIM_DisconnectParent(struct IClass *cl, Object *obj,
    struct MUIP_DisconnectParent *msg)
{
    struct Poppen_DATA *data = INST_DATA(cl, obj);

    if (data->wnd)
        DoMethod(obj, MUIM_Poppen_CloseWindow, FALSE);
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

#if ZUNE_BUILTIN_POPPEN
/*
 * Dispatch local methods and return their results, delegating other
 * messages to the superclass. Requires ABI-correct messages and a live
 * object except construction. Owned and borrowed resources follow the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Poppen_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Poppen__OM_NEW(cl, obj, (struct opSet *)msg);
    case MUIM_Hide:
        return Poppen__MUIM_Hide(cl, obj, (APTR) msg);
    case MUIM_Poppen_OpenWindow:
        return Poppen__MUIM_Poppen_OpenWindow(cl, obj, (APTR) msg);
    case MUIM_Poppen_CloseWindow:
        return Poppen__MUIM_Poppen_CloseWindow(cl, obj, (APTR) msg);
    case MUIM_DisconnectParent:
        return Poppen__MUIM_DisconnectParent(cl, obj, (APTR) msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Poppen_desc =
{
    MUIC_Poppen,
    MUIC_Pendisplay,
    sizeof(struct Poppen_DATA),
    (void *) Poppen_Dispatcher
};
#endif /* ZUNE_BUILTIN_POPPEN */
