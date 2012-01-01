/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

#include <exec/rawfmt.h>
#include <libraries/mui.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <zune/customclasses.h>

#include <string.h>

#include "locale.h"

#include "smselector.h"

struct ScreenModeSelector_DATA
{
    STRPTR *modes_array;
    ULONG  *ids_array;
};

#define HOOK(name) \
struct Hook 

#define HOOKFUNC(name) IPTR name ## Func(struct Hook *hook, APTR obj, APTR msg)

AROS_UFH3(IPTR, SelectFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR         , obj , A2),
AROS_UFHA(APTR         , msg , A1))
{
    AROS_USERFUNC_INIT

    struct ScreenModeSelector_DATA *data = INST_DATA(OCLASS(obj), obj);    

    return set(obj, MUIA_ScreenModeSelector_Active, data->ids_array[*(IPTR *)msg]);
    
    AROS_USERFUNC_EXIT;
}
static struct Hook SelectHook = { .h_Entry = SelectFunc };

AROS_UFH3(IPTR, DisplayFunc,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(CONST_STRPTR *, array, A2),
AROS_UFHA(STRPTR, entry, A1))
{
    AROS_USERFUNC_INIT

    if (entry)
    {
        ULONG *ids_array = hook->h_Data;
        ULONG num = (ULONG)(IPTR)array[-1];
        IPTR modeid = ids_array[num];
        static char modeid_str[9];
    
        RawDoFmt("%08lx", &modeid, RAWFMTFUNC_STRING, modeid_str);
        array[0] = modeid_str;
        array[1] = entry;
    }
    else
    {
        array[0] = _(MSG_MODE_ID);
        array[1] = _(MSG_DESCRIPTION);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

static struct Hook DisplayHook = { .h_Entry = DisplayFunc };

Object *ScreenModeSelector__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    STRPTR *modes_array;
    ULONG  *ids_array;
    ULONG   id, num_modes, cur_mode;
    Object *list;

    struct DisplayInfo    DisplayInfo;
    struct DimensionInfo  DimensionInfo;
    struct NameInfo       NameInfo;
    APTR handle;

    struct ScreenModeSelector_DATA *data;

    num_modes = 0; id = INVALID_ID;
    while ((id = NextDisplayInfo(id)) != INVALID_ID) num_modes++;

    modes_array = AllocVec(sizeof(STRPTR) * (num_modes + 1), MEMF_CLEAR);
    if (!modes_array)
        goto err;

    ids_array   = AllocVec(sizeof(ULONG) * (num_modes + 1), MEMF_ANY);
    if (!ids_array)
        goto err;

    ids_array[num_modes] = INVALID_ID;

    cur_mode = 0;
    while ((id = NextDisplayInfo(id)) != INVALID_ID)
    {
        if ((id & MONITOR_ID_MASK) == DEFAULT_MONITOR_ID)
            continue;

        if (!(handle = FindDisplayInfo(id)))
            continue;

        if (!GetDisplayInfoData(handle, (UBYTE *)&NameInfo, sizeof(struct NameInfo), DTAG_NAME, 0))
            continue;

        if (!GetDisplayInfoData(handle, (UBYTE *)&DisplayInfo, sizeof(struct DisplayInfo), DTAG_DISP, 0))
            continue;
        
        if (!(DisplayInfo.PropertyFlags & DIPF_IS_WB) || DisplayInfo.NotAvailable)
            continue;

        if (!GetDisplayInfoData(handle, (UBYTE *)&DimensionInfo, sizeof(struct DimensionInfo), DTAG_DIMS, 0))
            continue;

        modes_array[cur_mode] = AllocVec(sizeof(NameInfo.Name), MEMF_ANY);
        if (!modes_array[cur_mode])
           continue;

        CopyMem(NameInfo.Name, modes_array[cur_mode], sizeof(NameInfo.Name));
        ids_array[cur_mode] = id;

        cur_mode++;
    }

    DisplayHook.h_Data = ids_array;

    list = (Object *)ListObject,
        InputListFrame,
        MUIA_List_DisplayHook, (IPTR)&DisplayHook,
        MUIA_List_Format, (IPTR)"BAR,",
        MUIA_List_SourceArray, (IPTR)modes_array,
        MUIA_List_Title, TRUE,
        MUIA_CycleChain, TRUE, /* CHECKME: Keyboard input in the list doesn't work, why? */
    End;

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_Listview_List, (IPTR)list,
        TAG_MORE,           (IPTR)message->ops_AttrList
    );

    if (!self)
        goto err;

    DoMethod(self, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
             (IPTR)self, 3, MUIM_CallHook, (IPTR)&SelectHook, MUIV_TriggerValue);
 
    data = INST_DATA(CLASS, self);    
    data->modes_array = modes_array;
    data->ids_array   = ids_array;

    return self;
    
err:
    CoerceMethod(CLASS, self, OM_DISPOSE);
    return NULL;
}

IPTR ScreenModeSelector__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct ScreenModeSelector_DATA *data;
    ULONG cur_mode;

    data = INST_DATA(CLASS, self);    

    if (data->modes_array)
    {
        for (cur_mode = 0; data->modes_array[cur_mode]; cur_mode++)
            FreeVec(data->modes_array[cur_mode]);

        FreeVec(data->modes_array);
    }

    FreeVec(data->ids_array);

    return DoSuperMethodA(CLASS, self, message);
}


IPTR ScreenModeSelector__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    struct ScreenModeSelector_DATA *data = INST_DATA(CLASS, self);    
    struct TagItem *tags;
    struct TagItem *tag;
    struct TagItem noforward_tags[] =
    {
        {MUIA_Group_Forward , FALSE                       },
        {TAG_MORE           , (IPTR)message->ops_AttrList }
    };
    struct opSet noforward_message = *message;
    noforward_message.ops_AttrList = noforward_tags;

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_ScreenModeSelector_Active:
            {
                int i;

                D(bug("[smselector] Set Active ID 0x%08lX\n", tag->ti_Data));
                for
                (
                    i = 0;
                    data->ids_array[i] != tag->ti_Data && data->ids_array[i] != INVALID_ID;
                    i++
                );
                
                if (data->ids_array[i] == INVALID_ID)
                    tag->ti_Data = INVALID_ID;
                else
                {
                    if (XGET(self, MUIA_List_Active) != i)
                    {
                        D(bug("[smselector] Set active item %lu\n", i));
                        NNFSET(self, MUIA_List_Active, i);
                    }
                }
                break;
            }
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg)&noforward_message);
}

IPTR ScreenModeSelector__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    struct ScreenModeSelector_DATA *data = INST_DATA(CLASS, self);    
    
    switch (message->opg_AttrID)
    {
        case MUIA_ScreenModeSelector_Active:
            *message->opg_Storage = data->ids_array[XGET(self, MUIA_Cycle_Active)];
            break;
        default:
            return DoSuperMethodA(CLASS, self, (Msg)message);
    }
    
    return TRUE;
}


ZUNE_CUSTOMCLASS_4
(
    ScreenModeSelector, NULL, MUIC_Listview, NULL,   
    OM_NEW,     struct opSet *,
    OM_DISPOSE, Msg,
    OM_GET,     struct opGet *,
    OM_SET,     struct opSet *
);
