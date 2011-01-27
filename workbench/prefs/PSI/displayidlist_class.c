/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#define DEBUG 0
#include "aros/debug.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/alib.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>

#include "displayidlist_class.h"

/****************************************************************************************/

struct DispIDlist_Data
{
    ULONG CurrentID;
    struct Hook ConstructHook;
    struct Hook DisplayHook;
};

/****************************************************************************************/

IPTR DispIDlist_DisplayFunc(struct Hook *hook, char **array, struct NameInfo *ni)
{
    *array = ni->Name;
    return 0;
}

/****************************************************************************************/

IPTR DispIDlist_CompareFunc(struct Hook *hook, struct NameInfo *n1, struct NameInfo *n2)
{
    return stricmp(n1->Name, n2->Name);
}

/****************************************************************************************/

VOID DispIDlist_DestructFunc(struct Hook *hook, APTR pool, struct NameInfo *ni)
{
    FreeVec(ni);
}

/****************************************************************************************/

IPTR DispIDlist_ConstructFunc(struct Hook *hook, APTR pool, ULONG modeid)
{
    APTR handle;
    struct NameInfo NameInfo;
    struct DisplayInfo DisplayInfo;
    struct DimensionInfo DimensionInfo;
    struct NameInfo *ni;

    if ((modeid & MONITOR_ID_MASK) == DEFAULT_MONITOR_ID)
        return 0;

    if (!(handle = FindDisplayInfo(modeid)))
        return 0;

    if (!GetDisplayInfoData(handle, (char *)&NameInfo, sizeof(struct NameInfo), DTAG_NAME, 0))
        return 0;

    if (!GetDisplayInfoData(handle, (char *)&DisplayInfo, sizeof(struct DisplayInfo), DTAG_DISP, 0))
        return 0;

    if (!GetDisplayInfoData(handle, (char *)&DimensionInfo, sizeof(struct DimensionInfo), DTAG_DIMS, 0))
        return 0;

    if (!(DisplayInfo.PropertyFlags & DIPF_IS_WB))
        return 0;

    if (DisplayInfo.NotAvailable)
        return 0;

    if (!(ni = AllocVec(sizeof(struct NameInfo), MEMF_ANY)))
        return 0;

    *ni = NameInfo;
    return (IPTR)ni;
}

/****************************************************************************************/

IPTR DispIDlist_New(struct IClass *cl,Object *obj,Msg msg)
{
    static struct Hook ConstructHook;
    ConstructHook.h_Entry = HookEntry;
    ConstructHook.h_SubEntry = DispIDlist_ConstructFunc;
    static struct Hook DestructHook;
    DestructHook.h_Entry = HookEntry;
    DestructHook.h_SubEntry = (HOOKFUNC)DispIDlist_DestructFunc;
    static struct Hook CompareHook;
    CompareHook.h_Entry = HookEntry;
    CompareHook.h_SubEntry = DispIDlist_CompareFunc;
    static struct Hook DisplayHook;
    DisplayHook.h_Entry = HookEntry;
    DisplayHook.h_SubEntry = DispIDlist_DisplayFunc;
    LONG id = INVALID_ID;

    if (!(obj=(Object *)DoSuperMethodA(cl,obj,msg)))
        return 0;

    SetSuperAttrs(cl, obj,
        MUIA_List_ConstructHook, &ConstructHook,
        MUIA_List_DestructHook , &DestructHook,
        MUIA_List_CompareHook  , &CompareHook,
        MUIA_List_DisplayHook  , &DisplayHook,
        MUIA_List_AutoVisible  , TRUE,
    TAG_DONE);

    while ((id = NextDisplayInfo(id)) != INVALID_ID)
        DoMethod(obj, MUIM_List_InsertSingle, id, MUIV_List_Insert_Bottom);

    DoMethod(obj, MUIM_List_Sort);

    DoMethod(obj, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MUIM_DispIDlist_Change);

    return (IPTR)obj;
}

/****************************************************************************************/

IPTR DispIDlist_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct DispIDlist_Data *data = INST_DATA(cl, obj);
    struct TagItem *tag,*quiet;

    quiet = FindTagItem(MUIA_DispIDlist_Quiet, msg->ops_AttrList);

    if ((tag = FindTagItem(MUIA_DispIDlist_CurrentID, msg->ops_AttrList)))
    {
        data->CurrentID = tag->ti_Data;

        if (!quiet)
        {
            int i;
            struct NameInfo *ni;
            ULONG mask = 0;

            for (;;)
            {
                for (i = 0; ; i++)
                {
                    DoMethod(obj, MUIM_List_GetEntry, i, &ni);
                    if (!ni)
                        break;
                    if ((ni->Header.DisplayID & ~mask) == (data->CurrentID & ~mask))
                    {
                        mask = MONITOR_ID_MASK;
                        break;
                    }
                }
                if (!ni)
                    break;
                if (mask == MONITOR_ID_MASK)
                    break;
                mask = MONITOR_ID_MASK;
            }

            if (ni)
                set(obj, MUIA_List_Active, i);
            else
                set(obj, MUIA_List_Active, MUIV_List_Active_Off);
        }
    }
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/****************************************************************************************/

IPTR DispIDlist_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct DispIDlist_Data *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
        case MUIA_DispIDlist_CurrentID:
            *(msg->opg_Storage) = data->CurrentID;
            return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

/****************************************************************************************/

IPTR DispIDlist_Change(struct IClass *cl, Object *obj, Msg msg)
{
    struct NameInfo *ni;
    DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &ni);
    D(bug("[PSI/DispIDlist_Change] ni %p id %d\n", ni, ni ? ni->Header.DisplayID : INVALID_ID));
    SetAttrs(obj,
        MUIA_DispIDlist_Quiet, TRUE,
        MUIA_DispIDlist_CurrentID, ni ? ni->Header.DisplayID : INVALID_ID,
    TAG_DONE);

    return 0;
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, DispIDlist_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return DispIDlist_New(cl, obj, (APTR)msg);
        case OM_SET: return DispIDlist_Set(cl, obj, (APTR)msg);
        case OM_GET: return DispIDlist_Get(cl, obj, (APTR)msg);
        case MUIM_DispIDlist_Change: return DispIDlist_Change(cl, obj, (APTR)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID DispIDlist_Init(VOID)
{
    CL_DispIDlist = MUI_CreateCustomClass
    (
        NULL, MUIC_List, NULL, sizeof(struct DispIDlist_Data), DispIDlist_Dispatcher
    );
}

/****************************************************************************************/

VOID DispIDlist_Exit(VOID)
{
    if (CL_DispIDlist) MUI_DeleteCustomClass(CL_DispIDlist);
}
