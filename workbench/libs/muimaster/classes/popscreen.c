/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "debug.h"
#include "popscreen_private.h"

/****** Popscreen.mui/MUIC_Popscreen ************
*
*   NAME
*       MUIC_Popscreen
*
*   SUPERCLASS
*       Popobject.mui
*
*   DESCRIPTION
*       Select a public-screen name through a Listview popup. Construction
*       creates List/Listview, installs embedded conversion hooks after
*       superclass construction and registers double-click acceptance. Supply
*       the inherited Popstring button and a live String-compatible object. No
*       screen is opened, selected as the application's screen, or retained by
*       this class.
*
*   METHODS
*       Before each normal opening, clear the list, lock the public-screen list,
*       append each node name in enumeration order and unlock. Select the first
*       case-insensitive match with String_Contents. If no match exists, a
*       nonempty string selects Active_Off; an empty string requests entry zero.
*       Return success regardless of insertion failures. There is no live
*       refresh while the popup stays open.
*       An accepted deferred close sets String_Contents from the active entry if
*       one exists. It does not set String_Acknowledge. Double-click requests
*       Close(TRUE); cancellation does not copy a selection. Window and
*       queued-hook lifetimes follow Popobject/Popstring.
*
*   NOTES
*       The List has no string construct/destruct hooks: inserted names remain
*       borrowed pointers into public-screen nodes after UnlockPubScreenList. No
*       individual public screen is locked for the popup lifetime. A screen
*       disappearing can invalidate its displayed name; this implementation does
*       not provide a stable copied snapshot. Selection of a name does not
*       ensure that screen still exists afterward.
*       Preserve the built-in popup arrangement; overriding Popobject_Object can
*       invalidate internal list assumptions. Constructor conversion hooks are
*       replaced by internal hooks. String_Contents must be readable and
*       non-NULL for comparison and the empty-string test.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Clear list, enumerate public-screen node names under list lock, unlock
 * and select first case-insensitive String match or first entry for empty
 * unmatched string. Return TRUE even on insertion failure. Names remain
 * borrowed after unlock; requires non-NULL readable string and stable nodes
 * during later use.
 */
LONG PopscreenStrObjFunc(struct Hook *hook, Object *popup, Object *str)
{
    struct Popscreen_DATA *data = (struct Popscreen_DATA *)hook->h_Data;
    struct List *pubscrlist;
    struct PubScreenNode *pubscrnode;
    STRPTR strtext = NULL, listentry;
    LONG index;

    DoMethod(data->list, MUIM_List_Clear);

    pubscrlist = LockPubScreenList();
    ForeachNode(pubscrlist, pubscrnode)
    {
        DoMethod(data->list, MUIM_List_InsertSingle,
            (IPTR) pubscrnode->psn_Node.ln_Name, MUIV_List_Insert_Bottom);
    }
    UnlockPubScreenList();

    get(str, MUIA_String_Contents, &strtext);

    for (index = 0;; index++)
    {
        DoMethod(data->list, MUIM_List_GetEntry, index, (IPTR) & listentry);

        if (!listentry)
        {
            set(data->list, MUIA_List_Active,
                strtext[0] ? MUIV_List_Active_Off : 0);
            break;
        }

        if (stricmp(strtext, listentry) == 0)
        {
            set(data->list, MUIA_List_Active, index);
            break;
        }
    }

    return TRUE;
}

/*
 * Retrieve active entry through popup Listview and set String_Contents when
 * present. Requires live popup/string and still-valid borrowed screen name.
 * No Acknowledge, screen lock or screen selection is performed; no return
 * status.
 */
void PopscreenObjStrFunc(struct Hook *hook, Object *popup, Object *str)
{
    STRPTR listentry;

    DoMethod(popup, MUIM_List_GetEntry, MUIV_List_GetEntry_Active,
        (IPTR) & listentry);

    if (listentry)
    {
        set(str, MUIA_String_Contents, listentry);
    }
}

/*
 * Construct List/Listview popup, install embedded conversion hooks and
 * double-click Close(TRUE) notification; return superclass object or NULL.
 * Requires inherited button/string. Preserve child arrangement and follow
 * Popobject window ownership; no list string-copy hooks are installed.
 */
IPTR Popscreen__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object *lv, *list;

    obj = (Object *) DoSuperNewTags
        (cl, obj, NULL,
        MUIA_Popobject_Object, (IPTR) (lv = (Object *) ListviewObject,
            MUIA_Listview_List, (IPTR) (list = (Object *) ListObject,
                InputListFrame,
                End),
            End),
        TAG_MORE, (IPTR) msg->ops_AttrList);

    if (obj)
    {
        struct Popscreen_DATA *data = INST_DATA(cl, obj);

        data->list = list;

        data->strobj_hook.h_Entry = HookEntry;
        data->strobj_hook.h_SubEntry = (HOOKFUNC) PopscreenStrObjFunc;
        data->strobj_hook.h_Data = data;

        data->objstr_hook.h_Entry = HookEntry;
        data->objstr_hook.h_SubEntry = (HOOKFUNC) PopscreenObjStrFunc;
        data->objstr_hook.h_Data = data;

        SetAttrs(obj, MUIA_Popobject_StrObjHook, (IPTR) & data->strobj_hook,
            MUIA_Popobject_ObjStrHook, (IPTR) & data->objstr_hook,
            TAG_DONE);

        DoMethod(lv, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            (IPTR) obj, 2, MUIM_Popstring_Close, TRUE);
    }

    return (IPTR) obj;
}

#if ZUNE_BUILTIN_POPSCREEN
/*
 * Dispatch local overrides and return their results, delegating other
 * messages to superclass. Requires ABI-correct message and live object
 * except construction. Resources and deferred callbacks follow the class
 * lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Popscreen_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Popscreen__OM_NEW(cl, obj, (struct opSet *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Popscreen_desc =
{
    MUIC_Popscreen,
    MUIC_Popobject,
    sizeof(struct Popscreen_DATA),
    (void *) Popscreen_Dispatcher
};
#endif /* ZUNE_BUILTIN_POPSCREEN */
