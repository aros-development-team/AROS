/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Services preferences - the editor: the list of services
          offered through the Services Manager, the fields of the selected
          one, and whether the Envoy servers start at boot.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <zune/customclasses.h>
#include <zune/prefseditor.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <string.h>

#include <aros/debug.h>

#include "locale.h"
#include "prefs.h"
#include "svceditor.h"

struct SvcEditor_DATA
{
    Object *list, *add, *remove;
    Object *name, *path, *pathpop, *active, *autorun;
    struct Hook displayhook;
};

#define SETUP_INST_DATA struct SvcEditor_DATA *data = INST_DATA(CLASS, self)

/* list columns: name, active, library (the long path last so it never hides the others) */
AROS_UFH3S(LONG, DisplayFunc,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(CONST_STRPTR *, array, A2),
           AROS_UFHA(struct PrefsEntry *, e, A1))
{
    AROS_USERFUNC_INIT

    if (e)
    {
        array[0] = e->pe_Name;
        array[1] = e->pe_Active ? _(MSG_YES) : _(MSG_NO);
        array[2] = e->pe_Path;
    }
    else
    {
        array[0] = _(MSG_COL_NAME);
        array[1] = _(MSG_COL_ACTIVE);
        array[2] = _(MSG_COL_PATH);
    }
    return 0;

    AROS_USERFUNC_EXIT
}

static struct PrefsEntry *Selected(struct SvcEditor_DATA *data)
{
    struct PrefsEntry *e = NULL;

    DoMethod(data->list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&e);
    return e;
}

static void ShowEntry(struct SvcEditor_DATA *data, struct PrefsEntry *e)
{
    NNSET(data->name, MUIA_String_Contents, (IPTR)(e ? e->pe_Name : ""));
    NNSET(data->path, MUIA_String_Contents, (IPTR)(e ? e->pe_Path : ""));
    NNSET(data->active, MUIA_Selected, e ? e->pe_Active : FALSE);
    SET(data->name, MUIA_Disabled, e == NULL);
    SET(data->pathpop, MUIA_Disabled, e == NULL);
    SET(data->active, MUIA_Disabled, e == NULL);
    SET(data->remove, MUIA_Disabled, e == NULL);
}

/* the in-memory list -> the gadgets */
static void Prefs2Gadgets(struct SvcEditor_DATA *data)
{
    struct PrefsEntry *e;

    SET(data->list, MUIA_List_Quiet, TRUE);
    DoMethod(data->list, MUIM_List_Clear);
    ForeachNode(&SvcList, e)
        DoMethod(data->list, MUIM_List_InsertSingle, (IPTR)e, MUIV_List_Insert_Bottom);
    SET(data->list, MUIA_List_Quiet, FALSE);
    NNSET(data->autorun, MUIA_Selected, SvcAutoRun);
    SET(data->list, MUIA_List_Active, IsListEmpty(&SvcList) ? MUIV_List_Active_Off : 0);
    ShowEntry(data, Selected(data));
}

static void CopyString(char *dst, ULONG size, CONST_STRPTR src)
{
    ULONG n = src ? strlen(src) : 0;

    if (n > size - 1)
        n = size - 1;
    if (n)
        CopyMem((APTR)src, dst, n);
    dst[n] = '\0';
}

/*** Methods ****************************************************************/

Object *SvcEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *list, *add, *remove, *name, *path, *pathpop, *active, *autorun, *child;

    self = (Object *)DoSuperNewTags(CLASS, self, NULL,
        MUIA_PrefsEditor_Name, (IPTR)_(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR)PREFS_PATH,
        MUIA_PrefsEditor_IconTool, (IPTR)"SYS:Prefs/Envoy/Services",
        TAG_DONE);
    if (!self)
        return NULL;
    {
        SETUP_INST_DATA;

        data->displayhook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(DisplayFunc);

        child = VGroup,
            Child, (IPTR)VGroup,
                GroupFrameT(_(MSG_SERVICES)),
                Child, (IPTR)ListviewObject,
                    MUIA_CycleChain, 1,
                    MUIA_Listview_List, (IPTR)(list = (Object *)ListObject,
                        InputListFrame,
                        MUIA_List_Format, (IPTR)"BAR,BAR,",
                        MUIA_List_Title, TRUE,
                        MUIA_List_DisplayHook, (IPTR)&data->displayhook,
                    End),
                End,
                Child, (IPTR)HGroup,
                    MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(add = SimpleButton(_(MSG_ADD))),
                    Child, (IPTR)(remove = SimpleButton(_(MSG_REMOVE))),
                End,
                Child, (IPTR)ColGroup(2),
                    Child, (IPTR)Label2(_(MSG_NAME)),
                    Child, (IPTR)(name = (Object *)StringObject,
                        StringFrame,
                        MUIA_CycleChain, 1,
                        MUIA_String_MaxLen, PREFS_NAMESIZE,
                    End),
                    Child, (IPTR)Label2(_(MSG_PATH)),
                    Child, (IPTR)(pathpop = (Object *)PopaslObject,
                        MUIA_Popasl_Type, ASL_FileRequest,
                        ASLFR_TitleText, (IPTR)_(MSG_ASLTITLE),
                        ASLFR_InitialDrawer, (IPTR)"SYS:System/Network/Envoy/Services",
                        ASLFR_InitialPattern, (IPTR)"#?.service",
                        MUIA_Popstring_String, (IPTR)(path = (Object *)StringObject,
                            StringFrame,
                            MUIA_CycleChain, 1,
                            MUIA_String_MaxLen, PREFS_PATHSIZE,
                        End),
                        MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopFile),
                    End),
                    Child, (IPTR)Label1(_(MSG_ACTIVE)),
                    Child, (IPTR)HGroup,
                        Child, (IPTR)(active = MUI_MakeObject(MUIO_Checkmark, NULL)),
                        Child, (IPTR)HVSpace,
                    End,
                End,
            End,
            Child, (IPTR)HGroup,
                Child, (IPTR)(autorun = MUI_MakeObject(MUIO_Checkmark, NULL)),
                Child, (IPTR)LLabel1(_(MSG_AUTORUN)),
                Child, (IPTR)HVSpace,
            End,
        End;

        if (!child)
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
        }
        data->list = list;
        data->add = add;
        data->remove = remove;
        data->name = name;
        data->path = path;
        data->pathpop = pathpop;
        data->active = active;
        data->autorun = autorun;
        set(active, MUIA_CycleChain, 1);
        set(autorun, MUIA_CycleChain, 1);

        DoMethod(self, OM_ADDMEMBER, (IPTR)child);

        DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)self, 1, MUIM_SvcEditor_Select);
        DoMethod(name, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, (IPTR)self, 1, MUIM_SvcEditor_Update);
        DoMethod(path, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, (IPTR)self, 1, MUIM_SvcEditor_Update);
        DoMethod(active, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR)self, 1, MUIM_SvcEditor_Update);
        DoMethod(autorun, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, (IPTR)self, 1, MUIM_SvcEditor_AutoRun);
        DoMethod(add, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)self, 1, MUIM_SvcEditor_Add);
        DoMethod(remove, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)self, 1, MUIM_SvcEditor_Remove);

        Prefs2Gadgets(data);
    }
    return self;
}

IPTR SvcEditor__MUIM_SvcEditor_Select(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    ShowEntry(data, Selected(data));
    return 0;
}

IPTR SvcEditor__MUIM_SvcEditor_Update(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct PrefsEntry *e = Selected(data);

    if (!e)
        return 0;
    CopyString(e->pe_Name, sizeof(e->pe_Name), (CONST_STRPTR)XGET(data->name, MUIA_String_Contents));
    CopyString(e->pe_Path, sizeof(e->pe_Path), (CONST_STRPTR)XGET(data->path, MUIA_String_Contents));
    e->pe_Active = XGET(data->active, MUIA_Selected) ? TRUE : FALSE;
    DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_Active);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

IPTR SvcEditor__MUIM_SvcEditor_AutoRun(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    SvcAutoRun = XGET(data->autorun, MUIA_Selected) ? TRUE : FALSE;
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

IPTR SvcEditor__MUIM_SvcEditor_Add(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct PrefsEntry *e;

    if (!(e = AddPrefsEntry(&SvcList, _(MSG_NEWSERVICE), "SYS:System/Network/Envoy/Services/", TRUE)))
        return 0;
    DoMethod(data->list, MUIM_List_InsertSingle, (IPTR)e, MUIV_List_Insert_Bottom);
    SET(data->list, MUIA_List_Active, MUIV_List_Active_Bottom);
    SET(_win(self), MUIA_Window_ActiveObject, (IPTR)data->name);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

IPTR SvcEditor__MUIM_SvcEditor_Remove(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct PrefsEntry *e = Selected(data);

    if (!e)
        return 0;
    DoMethod(data->list, MUIM_List_Remove, MUIV_List_Remove_Active);
    Remove(&e->pe_Node);
    FreeVec(e);
    ShowEntry(data, Selected(data));
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

/*
 * Import/Export by name: called with the current directory set to ENV: or
 * ENVARC: and the relative path PREFS_PATH, or with any file the user picks.
 * AutoRun lives next to services.prefs and travels with it only in the
 * ENV:/ENVARC: case.
 */
static BOOL IsOurPath(CONST_STRPTR filename)
{
    return filename && !strcmp(filename, PREFS_PATH);
}

IPTR SvcEditor__MUIM_PrefsEditor_Import(Class *CLASS, Object *self, struct MUIP_PrefsEditor_Import *message)
{
    SETUP_INST_DATA;

    if (!Prefs_Load(message->filename))
        return FALSE;
    if (IsOurPath(message->filename))
        Prefs_LoadAutoRun(PREFS_AUTORUN);
    Prefs2Gadgets(data);
    return TRUE;
}

IPTR SvcEditor__MUIM_PrefsEditor_Export(Class *CLASS, Object *self, struct MUIP_PrefsEditor_Export *message)
{
    if (!Prefs_Save(message->filename))
        return FALSE;
    if (IsOurPath(message->filename))
        Prefs_SaveAutoRun(PREFS_AUTORUN);
    return TRUE;
}

IPTR SvcEditor__MUIM_PrefsEditor_ImportFH(Class *CLASS, Object *self, struct MUIP_PrefsEditor_ImportFH *message)
{
    SETUP_INST_DATA;

    if (!Prefs_ImportFH(message->fh))
        return FALSE;
    Prefs2Gadgets(data);
    return TRUE;
}

IPTR SvcEditor__MUIM_PrefsEditor_ExportFH(Class *CLASS, Object *self, struct MUIP_PrefsEditor_ExportFH *message)
{
    return Prefs_ExportFH(message->fh);
}

IPTR SvcEditor__MUIM_PrefsEditor_SetDefaults(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    if (!Prefs_Default())
        return FALSE;
    Prefs2Gadgets(data);
    return TRUE;
}

ZUNE_CUSTOMCLASS_11
(
    SvcEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                       struct opSet *,
    MUIM_SvcEditor_Select,        Msg,
    MUIM_SvcEditor_Update,        Msg,
    MUIM_SvcEditor_AutoRun,       Msg,
    MUIM_SvcEditor_Add,           Msg,
    MUIM_SvcEditor_Remove,        Msg,
    MUIM_PrefsEditor_Import,      struct MUIP_PrefsEditor_Import *,
    MUIM_PrefsEditor_Export,      struct MUIP_PrefsEditor_Export *,
    MUIM_PrefsEditor_ImportFH,    struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,    struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults, Msg
);
