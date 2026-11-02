/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <string.h>

/*  #define MYDEBUG 1 */
#include "debug.h"

#include "mui.h"
#include "muimaster_intern.h"
#include "prefs.h"
#include "support.h"
#include "support_classes.h"
#include "radio_private.h"

/****** Radio.mui/MUIC_Radio ************
*
*   NAME
*       MUIC_Radio
*
*   SUPERCLASS
*       Group.mui
*
*   DESCRIPTION
*       Display a set of mutually exclusive choices as groups of a radio image
*       and text label. Each image and label uses immediate input and routes
*       every Selected notification to the containing Radio's Active setter with
*       its own index. Clicking the label or image therefore selects the same
*       choice. The class owns the constructed child groups and an array of
*       child pointers.
*       Construction requires Entries. Each Text child copies its label; Radio
*       does not retain the input array. Initial Active defaults to zero and an
*       out-of-range value is replaced by zero. Child construction and
*       notification registration do not have comprehensive individual failure
*       checks. A NULL Entries argument follows an early disposal/failure path
*       before superclass construction; provide a valid nonempty array.
*       Setup delegates first, returns FALSE on superclass failure, then sets
*       horizontal and vertical Group spacing from the radio-button preferences
*       and returns TRUE. Dispose frees the pointer array and delegates child
*       disposal to Group. Serialize setters and child callbacks with disposal;
*       no additional lifetime or locking mechanism is provided.
*
*   METHODS
*       With a nonzero ObjectID, Export writes one LONG containing the current
*       selection to the supplied Dataspace. Import looks up that ID and sets
*       Active from the first LONG of the returned record. Both return zero and
*       ignore operation status; ObjectID zero performs no access. Use a live
*       Dataspace with a correctly sized LONG record: Import does not check the
*       stored length. Imported invalid indices follow the ordinary setter
*       behavior, including notification forwarding.
*
*/
/****** Radio.mui/MUIA_Radio_Entries ************
*
*   NAME
*       MUIA_Radio_Entries
*
*   FUNCTION
*       Create a radio choice for each supplied label.
*
*   INPUTS
*       Initialize only; pointer to a NULL-terminated array of pointers to
*       terminated strings. Required; no default array, local setter or local
*       getter.
*
*   NOTES
*       The array is read during construction and the Text children copy each
*       string. After successful construction the caller may release the array
*       and source strings. Radio owns the child groups; do not dispose them
*       separately. Replacing Entries on an existing Radio does not rebuild its
*       choices.
*       Supply at least one entry and a count representable by the constructor
*       allocations. An empty array does not produce a useful selected choice,
*       and zero-size allocation behavior can affect construction. Count
*       arithmetic and child allocation failures are not fully checked. Return
*       NULL from construction is not a guarantee that every intermediate
*       allocation was reclaimed.
*
*/
/****** Radio.mui/MUIA_Radio_Active ************
*
*   NAME
*       MUIA_Radio_Active
*
*   FUNCTION
*       Select a radio choice or return its stored zero-based index.
*
*   INPUTS
*       Initialize, set and get, LONG. Default zero. Valid indices are zero
*       through the number of entries minus one. No Next/Prev sentinel values
*       are implemented.
*
*   NOTES
*       Construction replaces invalid indices with zero. Set ignores an invalid
*       or unchanged index locally. For a valid change it sends NoNotifySet
*       Selected FALSE to the old child group and TRUE to the new one, then
*       stores the new index. Group forwarding updates the image and text
*       without recursively delivering their Selected notifications.
*       The original Active tag is still passed to Group even when no selection
*       changes. Notifications can therefore report an invalid or unchanged
*       requested value; MUIV_TriggerValue is not proof of the current
*       selection. Read Active to obtain the stored index. Get stores that index
*       and returns TRUE. Set returns the superclass result and supplies no
*       separate change/error status.
*
*/

extern struct Library *MUIMasterBase;

/*
 * Read required NULL-terminated nonempty Entries and initial Active; create
 * owned choice children and pointer array, then register selection routing.
 * Labels are copied by Text children; input array is not retained. Return
 * object or zero on failure, with incomplete checks/rollback for
 * intermediate child creation. NULL Entries takes a pre-super disposal
 * path.
 */
IPTR Radio__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Radio_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;
    int i;
    const char **entries = NULL;
    int entries_active = 0;
    int entries_num;
    struct TagItem *grouptags;
    Object **buttons, **imgobjs, **textobjs;
    int state;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Radio_Entries:
            entries = (const char **)tag->ti_Data;
            break;
        case MUIA_Radio_Active:
            entries_active = tag->ti_Data;
            break;
        }
    }

    if (!entries)
    {
        D(bug("Radio_New: No Entries specified!\n"));
        CoerceMethod(cl, obj, OM_DISPOSE);
        return 0;
    }

    /* Count the number of entries */
    for (i = 0; entries[i]; i++);

    entries_num = i;

    if ((entries_active < 0) || (entries_active > entries_num - 1))
        entries_active = 0;

    grouptags = AllocateTagItems(entries_num + 1);
    if (!grouptags)
        return FALSE;
    buttons = AllocVec(i * sizeof(Object *) * 3, MEMF_PUBLIC);
    if (!buttons)
    {
        FreeVec(grouptags);
        return FALSE;
    }
    imgobjs = buttons + i;
    textobjs = imgobjs + i;

    for (i = 0; i < entries_num; i++)
    {
        state = (entries_active == i) ? TRUE : FALSE;

        buttons[i] = HGroup,
            Child, (IPTR)(imgobjs[i] = ImageObject,
                MUIA_Image_FontMatch, TRUE,
                MUIA_InputMode, MUIV_InputMode_Immediate,
                MUIA_Selected, state,
                MUIA_ShowSelState, FALSE,
                MUIA_Image_Spec, MUII_RadioButton,
                MUIA_Frame, MUIV_Frame_None,
                   End),
            Child, (IPTR)(textobjs[i] = TextObject,
                MUIA_InputMode, MUIV_InputMode_Immediate,
                MUIA_ShowSelState, FALSE,
                MUIA_Selected, state,
                MUIA_Text_Contents, entries[i],
                MUIA_Frame, MUIV_Frame_None,
                MUIA_Text_PreParse, (IPTR)"\33l",
                End),
            End;

        grouptags[i].ti_Tag = MUIA_Group_Child;
        grouptags[i].ti_Data = (IPTR) buttons[i];
    }

    grouptags[i].ti_Tag = TAG_MORE;
    grouptags[i].ti_Data = (IPTR) msg->ops_AttrList;

    obj = (Object *) DoSuperNewTags(cl, obj, NULL,
        TAG_MORE, (IPTR) grouptags);
    FreeTagItems(grouptags);
    if (!obj)
    {
        FreeVec(buttons);
        return FALSE;
    }
    data = INST_DATA(cl, obj);
    data->entries_active = entries_active;
    data->entries_num = entries_num;
    data->buttons = buttons;

    for (i = 0; i < entries_num; i++)
    {
        DoMethod(imgobjs[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) obj, 3, MUIM_Set, MUIA_Radio_Active, i);
        DoMethod(textobjs[i], MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
            (IPTR) obj, 3, MUIM_Set, MUIA_Radio_Active, i);
    }

    return (IPTR) obj;
}

/*
 * Free local child-pointer array if present, then delegate owned child
 * disposal to Group and return its result. Requires no concurrent selection
 * callbacks. Does not free caller Entries storage because it was not
 * retained.
 */
IPTR Radio__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct Radio_DATA *data = INST_DATA(cl, obj);
    if (data->buttons)
        FreeVec(data->buttons);
    return DoSuperMethodA(cl, obj, msg);
}

/*
 * For a changed in-range index, NoNotifySet old/new child group selection
 * and store Active. Invalid/unchanged values leave local state intact but
 * original tags still reach Group notifications. Requires live children and
 * valid tag list; return superclass result, not a selection-change
 * indication.
 */
IPTR Radio__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Radio_DATA *data;
    struct TagItem *tags;
    struct TagItem *tag;

    data = INST_DATA(cl, obj);

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Radio_Active:
            D(bug("Radio_Set(%p) MUIA_Radio_Active %ld\n",
                    obj, tag->ti_Data));
            if (tag->ti_Data >= 0 && tag->ti_Data < data->entries_num &&
                tag->ti_Data != data->entries_active)
            {
                DoMethod(data->buttons[data->entries_active],
                    MUIM_NoNotifySet, MUIA_Selected, FALSE);
                DoMethod(data->buttons[tag->ti_Data],
                    MUIM_NoNotifySet, MUIA_Selected, TRUE);
                data->entries_active = tag->ti_Data;
            }
            break;
        }
    }
    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Store current Active through writable opg_Storage and return TRUE;
 * delegate other attributes to Group. There is no local Entries getter.
 * Reading Active does not prove a corresponding child exists when
 * construction produced no entries.
 */
IPTR Radio__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Radio_DATA *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch (msg->opg_AttrID)
    {
    case MUIA_Radio_Active:
        STORE = data->entries_active;
        return 1;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/*
 * Delegate Setup and return FALSE if it fails. Otherwise synchronously set
 * Group horizontal/vertical spacing from global radio preferences and
 * return TRUE. Requires connected render/global context; setters may notify
 * and their results are ignored.
 */
IPTR Radio__MUIM_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    if (!DoSuperMethodA(cl, obj, msg))
        return FALSE;

    set(obj, MUIA_Group_HorizSpacing,
        muiGlobalInfo(obj)->mgi_Prefs->radiobutton_hspacing);
    set(obj, MUIA_Group_VertSpacing,
        muiGlobalInfo(obj)->mgi_Prefs->radiobutton_vspacing);

    return TRUE;
}

/**************************************************************************
 MUIM_Export - to export an object's "contents" to a dataspace object.
**************************************************************************/
/*
 * For nonzero ObjectID, synchronously copy one stack LONG selection into
 * live Dataspace via Add. Return zero regardless of Add result; do nothing
 * for ID zero. Caller owns Dataspace and must keep it alive through return.
 */
IPTR Radio__MUIM_Export(struct IClass *cl, Object *obj,
    struct MUIP_Export *msg)
{
    struct Radio_DATA *data = INST_DATA(cl, obj);
    ULONG id;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        LONG value = data->entries_active;
        DoMethod(msg->dataspace, MUIM_Dataspace_Add,
            (IPTR) & value, sizeof(value), (IPTR) id);
    }
    return 0;
}

/**************************************************************************
 MUIM_Import - to import an object's "contents" from a dataspace object.
**************************************************************************/
/*
 * For nonzero ObjectID, find a borrowed record in live Dataspace and set
 * Active from its first LONG when present. Requires at least sizeof(LONG)
 * readable bytes; no length check. Return zero even for no record or
 * rejected index; setter can notify synchronously.
 */
IPTR Radio__MUIM_Import(struct IClass *cl, Object *obj,
    struct MUIP_Import *msg)
{
    ULONG id;
    LONG *s;

    if ((id = muiNotifyData(obj)->mnd_ObjectID))
    {
        if ((s = (LONG *) DoMethod(msg->dataspace, MUIM_Dataspace_Find,
                    (IPTR) id)))
        {
            set(obj, MUIA_Radio_Active, *s);
        }
    }
    return 0;
}

#if ZUNE_BUILTIN_RADIO
/*
 * Dispatch local class overrides and return handler results; delegate other
 * messages to Group. Requires ABI-correct message and live object except
 * construction. Child ownership, event callbacks and any popup resources
 * follow the class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, Radio_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Radio__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Radio__OM_DISPOSE(cl, obj, (Msg) msg);
    case OM_SET:
        return Radio__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Radio__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Radio__MUIM_Setup(cl, obj, msg);
    case MUIM_Export:
        return Radio__MUIM_Export(cl, obj, (struct MUIP_Export *) msg);
    case MUIM_Import:
        return Radio__MUIM_Import(cl, obj, (struct MUIP_Import *) msg);
    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Radio_desc =
{
    MUIC_Radio,
    MUIC_Group,
    sizeof(struct Radio_DATA),
    (void *) Radio_Dispatcher
};
#endif /* ZUNE_BUILTIN_RADIO */
