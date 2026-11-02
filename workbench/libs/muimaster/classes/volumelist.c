/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>
#include <stdio.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"
#include "volumelist_private.h"
#include "locale.h"

/****** Volumelist.mui/MUIC_Volumelist ************
*
*   NAME
*       MUIC_Volumelist
*
*   SUPERCLASS
*       List.mui
*
*   DESCRIPTION
*       Populate a list with devices, volumes and assigns from the DOS list at
*       construction. This class has no additional public attributes or methods.
*       Entries are appended in three passes: devices, volumes, then assigns, in
*       their DOS-list traversal order. No sorting, periodic refresh or
*       disk-change handler is installed.
*       The default format has five columns: icon, name, percentage used, free
*       space and used space, with numeric columns right-aligned. A non-NULL
*       caller List_Format suppresses this default. Construction installs its
*       own construction, destruction and display hooks after superclass
*       creation. Preserve those hooks for the class's private entry records;
*       entries are not ordinary strings.
*
*   METHODS
*       Construction locks the DOS list for reading devices, volumes and
*       assigns. For each volume it additionally locks its colon-suffixed name,
*       requests Info, and releases that volume lock. It releases the DOS-list
*       lock after all passes. These synchronous DOS operations can block; no
*       timeout or cancellation is implemented. It returns the superclass object
*       even if individual row insertions fail, because insertion results are
*       ignored.
*       Device and assign rows leave usage fields empty. Volume rows also leave
*       them empty when Lock or Info fails. Successful Info supplies percentage
*       and byte counts. Percentage calculation does not guard a zero block
*       count or widen its multiplication before division; inconsistent or large
*       filesystem statistics can produce invalid arithmetic. Byte products use
*       UQUAD, but inconsistent used/total values can underflow the free-space
*       subtraction.
*
*   NOTES
*       Names are stored in a 100-byte inline buffer: at most 98 copied name
*       bytes followed by a colon and terminator. The truncated name is also
*       used for volume Lock. Size strings use integer truncation: B through
*       9999 bytes, K above 9999 using a ten-bit shift, M from 9999999 using a
*       twenty-bit shift, and G from 9999999999 using a thirty-bit shift. The
*       final numeric value is cast to unsigned int; exceptionally large values
*       can truncate again.
*       Each inserted private record is copied into List's pool and released
*       there by the destruction hook. Display returns pointers into that
*       record, static icon strings or localized heading strings; it transfers
*       no ownership. The class does not retain DOS-list entries or volume locks
*       after construction. Inherited List operations remain available, but no
*       class-specific refresh operation repopulates the snapshot.
*
*/

extern struct Library *MUIMasterBase;


/*
 * Format byte count with integer B/K/M/G units into caller-owned buffer and
 * force final NUL. Requires bufsize greater than zero and writable storage.
 * Shifts truncate fractions and unsigned-int conversion can truncate large
 * results; no allocation or status.
 */
static void printSize(STRPTR string, size_t bufsize, UQUAD size)
{
    char unit = 'B';

    if (size >= 9999999999ULL)
    {
        size = size >> 30;
        unit = 'G';
    }
    else if (size >= 9999999UL)
    {
        size = size >> 20;
        unit = 'M';
    }
    else if (size > 9999)
    {
        size = size >> 10;
        unit = 'K';
    }

    snprintf(string, bufsize, "%u%c", (unsigned int)size, unit);
    string[bufsize - 1] = '\0';
}

/*
 * Copy private entry record into List pool allocation; return new owned
 * record or NULL on allocation failure. Requires valid pool and readable
 * complete source record; all string fields are inline. Hook argument is
 * unused.
 */
AROS_UFH3S(APTR, construct_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Volumelist_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    struct Volumelist_Entry *new;

    if ((new = AllocPooled(pool, sizeof(*new))))
    {
        *new = *entry;
    }
    return new;

    AROS_USERFUNC_EXIT
}

/*
 * Free a private entry record through the same List pool and exact record
 * size. Requires matching successful construction and no remaining display
 * consumers. No other resource or borrowed DOS object is freed.
 */
AROS_UFH3S(void, destruct_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Volumelist_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct Volumelist_Entry));

    AROS_USERFUNC_EXIT
}

/*
 * Fill five writable column-pointer slots from a live entry or localized
 * headings when entry is NULL. Return zero. Pointers remain borrowed; no
 * allocation or mutation of entry. Requires active locale and valid inline
 * strings.
 */
AROS_UFH3S(LONG, display_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(char **, array, A2),
    AROS_UFHA(struct Volumelist_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    /* MUI: logo | devicename | %-used | bytes free | bytes used */

    if (entry)
    {
        if (entry->type == DLT_DEVICE)
        {
            *array++ = "\33I[6:24]";
        }
        else if (entry->type == DLT_VOLUME)
        {
            *array++ = "\33I[6:26]";
        }
        else
        {
            *array++ = "\33I[6:29]";
        }

        *array++ = entry->name;
        *array++ = entry->full;
        *array++ = entry->free;
        *array = entry->used;
    }
    else
    {
        *array++ = "";
        *array++ = (char *)_(MSG_VOLUMELIST_NAME);
        *array++ = (char *)_(MSG_VOLUMELIST_FULL);
        *array++ = (char *)_(MSG_VOLUMELIST_FREE);
        *array = (char *)_(MSG_VOLUMELIST_USED);
    }

    return 0;

    AROS_USERFUNC_EXIT
}


/*
 * Construct List with overridable format, install private record hooks, and
 * populate DOS snapshot under read lock. Volume Lock/Info calls are
 * synchronous; release obtained locks. Return superclass object or NULL;
 * insertion failures ignored and filesystem arithmetic unchecked. No
 * persistent DOS-list pointer or lock ownership.
 */
IPTR Volumelist__OM_NEW(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct DosList *dl, *actdl;

    STRPTR format =
        (STRPTR) GetTagData(MUIA_List_Format, 0, msg->ops_AttrList);

    obj = (Object *)DoSuperNewTags
    (
        cl, obj, NULL,
        format ? TAG_IGNORE : MUIA_List_Format, (IPTR)",,P=\33r,P=\33r,P=\33r",
        TAG_MORE, (IPTR) msg->ops_AttrList
    );

    if (obj)
    {
        struct Volumelist_DATA *data = INST_DATA(cl, obj);

        data->construct_hook.h_Entry = (HOOKFUNC) construct_func;
        data->destruct_hook.h_Entry = (HOOKFUNC) destruct_func;
        data->display_hook.h_Entry = (HOOKFUNC) display_func;

        SetAttrs(obj, MUIA_List_ConstructHook,
            (IPTR) & data->construct_hook, MUIA_List_DestructHook,
            (IPTR) & data->destruct_hook, MUIA_List_DisplayHook,
            (IPTR) & data->display_hook, TAG_DONE);

        dl = LockDosList(LDF_READ | LDF_VOLUMES | LDF_ASSIGNS |
            LDF_DEVICES);

        actdl = dl;
        while ((actdl = NextDosEntry(actdl, LDF_DEVICES)))
        {
            struct Volumelist_Entry entry;

            entry.full[0] = '\0';
            entry.free[0] = '\0';
            entry.used[0] = '\0';

            entry.type = DLT_DEVICE;

            strncpy(entry.name, AROS_BSTR_ADDR(actdl->dol_Name),
                sizeof(entry.name));
            entry.name[sizeof(entry.name) - 2] = '\0';
            strcat(entry.name, ":");

            DoMethod(obj, MUIM_List_InsertSingle, (IPTR) & entry,
                MUIV_List_Insert_Bottom);
        }

        actdl = dl;
        while ((actdl = NextDosEntry(actdl, LDF_VOLUMES)))
        {
            struct Volumelist_Entry entry;
            struct InfoData diskinfo;
            BPTR lock;
            UQUAD free;
            UQUAD used;

            entry.full[0] = '\0';
            entry.free[0] = '\0';
            entry.used[0] = '\0';

            entry.type = DLT_VOLUME;

            strncpy(entry.name, AROS_BSTR_ADDR(actdl->dol_Name),
                sizeof(entry.name));
            entry.name[sizeof(entry.name) - 2] = '\0';
            strcat(entry.name, ":");

            if ((lock = Lock(entry.name, SHARED_LOCK)) != BNULL)
            {
                if (Info(lock, &diskinfo) != DOSFALSE)
                {
                    snprintf
                        (entry.full,
                        sizeof(entry.full),
                        "%ld%%",
                        (long)(100 * diskinfo.id_NumBlocksUsed /
                            diskinfo.id_NumBlocks));
                    entry.full[sizeof(entry.full) - 1] = '\0';

                    used =
                        (UQUAD) diskinfo.id_NumBlocksUsed *
                        diskinfo.id_BytesPerBlock;
                    free =
                        (UQUAD) diskinfo.id_NumBlocks *
                        diskinfo.id_BytesPerBlock - used;
                    printSize(entry.free, sizeof entry.free, free);
                    printSize(entry.used, sizeof entry.used, used);
                }
                UnLock(lock);
            }

            DoMethod(obj, MUIM_List_InsertSingle, (IPTR) & entry,
                MUIV_List_Insert_Bottom);
        }

        actdl = dl;
        while ((actdl = NextDosEntry(actdl, LDF_ASSIGNS)))
        {
            struct Volumelist_Entry entry;

            entry.full[0] = '\0';
            entry.free[0] = '\0';
            entry.used[0] = '\0';

            entry.type = DLT_DIRECTORY;

            strncpy(entry.name, AROS_BSTR_ADDR(actdl->dol_Name),
                sizeof(entry.name));
            entry.name[sizeof(entry.name) - 2] = '\0';
            strcat(entry.name, ":");

            DoMethod(obj, MUIM_List_InsertSingle, (IPTR) & entry,
                MUIV_List_Insert_Bottom);
        }

        UnLockDosList(LDF_READ | LDF_VOLUMES | LDF_ASSIGNS | LDF_DEVICES);
    }

    return (IPTR) obj;
}


#if ZUNE_BUILTIN_VOLUMELIST
/*
 * When built in, dispatch local overrides and return their results;
 * delegate other messages to List. Requires ABI-correct messages and live
 * object except construction. Ownership and lifecycle follow class
 * contract.
 */
BOOPSI_DISPATCHER(IPTR, Volumelist_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Volumelist__OM_NEW(cl, obj, (struct opSet *)msg);

    default:
        return DoSuperMethodA(cl, obj, msg);
    }
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Volumelist_desc =
{
    MUIC_Volumelist,
    MUIC_List,
    sizeof(struct Volumelist_DATA),
    (void *) Volumelist_Dispatcher
};
#endif /* ZUNE_BUILTIN_VOLUMELIST */
