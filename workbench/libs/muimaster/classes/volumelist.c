/*
    Copyright © 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
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

extern struct Library *MUIMasterBase;


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

AROS_UFH3S(void, destruct_func,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, pool, A2),
    AROS_UFHA(struct Volumelist_Entry *, entry, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(pool, entry, sizeof(struct Volumelist_Entry));

    AROS_USERFUNC_EXIT
}

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
        *array++ = "Name";
        *array++ = "full";
        *array++ = "free";
        *array = "used";
    }

    return 0;

    AROS_USERFUNC_EXIT
}


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
