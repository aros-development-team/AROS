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
#include "poplist_private.h"

/****** Poplist.mui/MUIC_Poplist ************
*
*   NAME
*       MUIC_Poplist
*
*   SUPERCLASS
*       Popobject.mui
*
*   DESCRIPTION
*       Provide a Listview popup backed by a List and synchronize its active
*       entry with a caller-supplied string. Construction creates the popup,
*       installs internal StrObjHook and ObjStrHook after superclass
*       construction, and registers double-click acceptance. It returns NULL on
*       superclass failure. Supply the inherited Popstring button and a
*       compatible String object; no string or button is created here.
*
*   METHODS
*       Before opening, compare String_Contents with entries using stricmp,
*       selecting the first case-insensitive match. If none matches, set
*       List_Active_Off and still allow opening. On accepted close, retrieve the
*       active entry through the Listview and set String_Contents only when an
*       entry exists. No String_Acknowledge is set here.
*       Double-click invokes Close(TRUE), whose actual copy occurs later through
*       Popstring's queued hook. Cancellation leaves the string unchanged. The
*       popup window and contents follow Popobject ownership and lifecycle
*       rules.
*
*   NOTES
*       The built-in List has no string construct/destruct hooks, so source
*       entry pointers remain borrowed rather than copied. Keep entry strings
*       valid while the popup uses them. Preserve the built-in popup/child
*       arrangement: supplying an overriding Popobject_Object can leave internal
*       list references inconsistent. Caller conversion hooks supplied at
*       construction are replaced by Poplist's internal hooks.
*
*/
/****** Poplist.mui/MUIA_Poplist_Array ************
*
*   NAME
*       MUIA_Poplist_Array
*
*   FUNCTION
*       Supply the initial list entries.
*
*   INPUTS
*       Initialize only, pointer to a NULL-terminated array of NUL-terminated
*       string pointers; default NULL for an empty list. No local setter or
*       getter.
*
*   NOTES
*       The array is passed to List_SourceArray during construction. The array
*       itself is consumed then; without construct hooks the entry strings
*       remain borrowed for subsequent display, comparisons and accepted
*       selection. This is not an owned copy of each string. Matching is
*       case-insensitive and selects the first match; a missing match does not
*       prevent opening.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Select first case-insensitive list entry matching live String_Contents,
 * or turn active selection off at end; return TRUE in either case. Requires
 * non-NULL readable string and stable list entries. No copying or ownership
 * transfer.
 */
LONG PoplistStrObjFunc(struct Hook *hook, Object *popup, Object *str)
{
    struct Poplist_DATA *data = (struct Poplist_DATA *)hook->h_Data;
    STRPTR strtext = NULL, listentry;
    LONG index;

    get(str, MUIA_String_Contents, &strtext);

    for (index = 0;; index++)
    {
        DoMethod(data->list, MUIM_List_GetEntry, index, (IPTR) & listentry);

        if (!listentry)
        {
            set(data->list, MUIA_List_Active, MUIV_List_Active_Off);
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
 * Fetch active entry through popup Listview and set String_Contents when
 * non-NULL. Requires live built-in popup and string; does not set
 * Acknowledge. Entry stays borrowed; no return status.
 */
void PoplistObjStrFunc(struct Hook *hook, Object *popup, Object *str)
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
 * Construct List/Listview popup from optional borrowed source array, then
 * install embedded conversion hooks and double-click acceptance
 * notification. Return superclass object or NULL. Requires inherited
 * button/string and stable entry strings; preserve built-in popup
 * arrangement.
 */
IPTR Poplist__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    STRPTR array =
        (STRPTR) GetTagData(MUIA_Poplist_Array, 0, msg->ops_AttrList);
    Object *lv, *list;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        MUIA_Popobject_Object, (IPTR) (lv = (Object *) ListviewObject,
            MUIA_Listview_List, (IPTR) (list = (Object *) ListObject,
                InputListFrame,
                array ? MUIA_List_SourceArray : TAG_IGNORE, (IPTR) array,
                End),
            End),
        TAG_MORE, (IPTR) msg->ops_AttrList);

    if (obj)
    {
        struct Poplist_DATA *data = INST_DATA(cl, obj);

        data->list = list;

        data->strobj_hook.h_Entry = HookEntry;
        data->strobj_hook.h_SubEntry = (HOOKFUNC) PoplistStrObjFunc;
        data->strobj_hook.h_Data = data;

        data->objstr_hook.h_Entry = HookEntry;
        data->objstr_hook.h_SubEntry = (HOOKFUNC) PoplistObjStrFunc;
        data->objstr_hook.h_Data = data;

        SetAttrs(obj, MUIA_Popobject_StrObjHook, (IPTR) & data->strobj_hook,
            MUIA_Popobject_ObjStrHook, (IPTR) & data->objstr_hook,
            TAG_DONE);

        DoMethod(lv, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            (IPTR) obj, 2, MUIM_Popstring_Close, TRUE);
    }

    return (IPTR) obj;
}

#if ZUNE_BUILTIN_POPLIST
/*
 * Dispatch local overrides and return their results; delegate other
 * messages to superclass. Requires ABI-correct message and live object
 * except construction. Deferred callbacks and resource ownership follow the
 * class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Poplist_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Poplist__OM_NEW(cl, obj, (struct opSet *)msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Poplist_desc =
{
    MUIC_Poplist,
    MUIC_Popobject,
    sizeof(struct Poplist_DATA),
    (void *) Poplist_Dispatcher
};
#endif /* ZUNE_BUILTIN_POPLIST */
