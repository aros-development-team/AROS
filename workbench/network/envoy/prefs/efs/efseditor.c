/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Exports - the editor class. A list of exports on the
          left; the selected export's name, path, security level, options
          and the users and groups allowed to mount it on the right.
          Access entries are stored as numeric IDs and shown by name
          through accounts.library.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <zune/customclasses.h>
#include <zune/prefseditor.h>
#include <libraries/asl.h>
#include <envoy/envoy.h>
#include <envoy/accounts.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/accounts.h>
#include <proto/envoy.h>

#include <stdio.h>
#include <string.h>

#include <aros/debug.h>

#include "locale.h"
#include "efsprefs.h"
#include "efseditor.h"

struct EfsEditor_DATA
{
    Object *list, *add, *remove;
    Object *detail, *name, *path, *security;
    Object *readonly, *removable, *exall, *snapshot, *leftout;
    Object *acclist, *accadd, *accremove;
    struct Export *cur;
    CONST_STRPTR seclabels[4];
    struct Hook disphook, accdisphook;
};

#define SETUP_INST_DATA struct EfsEditor_DATA *data = INST_DATA(CLASS, self)

/*** names of access entries ***********************************************/

static void ResolveNames(struct Export *e)
{
    ULONG i;

    for (i = 0; i < e->NumAccess; i++)
    {
        char *dst = e->AccessName[i];
        BOOL found = FALSE;

        if (AccountsBase)
        {
            if (e->AccessGroup[i])
            {
                struct GroupInfo *gi = AllocGroupInfo();
                if (gi)
                {
                    if (IDToGroup(e->AccessID[i], gi) == 0)
                    {
                        strncpy(dst, (char *)gi->gi_GroupName, sizeof(e->AccessName[i]) - 1);
                        dst[sizeof(e->AccessName[i]) - 1] = '\0';
                        found = TRUE;
                    }
                    FreeGroupInfo(gi);
                }
            }
            else
            {
                struct UserInfo *ui = AllocUserInfo();
                if (ui)
                {
                    if (IDToUser(e->AccessID[i], ui) == 0)
                    {
                        strncpy(dst, (char *)ui->ui_UserName, sizeof(e->AccessName[i]) - 1);
                        dst[sizeof(e->AccessName[i]) - 1] = '\0';
                        found = TRUE;
                    }
                    FreeUserInfo(ui);
                }
            }
        }
        if (!found)
            snprintf(dst, sizeof(e->AccessName[i]), "#%u", (unsigned)e->AccessID[i]);
    }
}

/*** list hooks ************************************************************/

AROS_UFH3S(IPTR, ExportDisplay,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(CONST_STRPTR *, array, A2),
           AROS_UFHA(struct Export *, e, A1))
{
    AROS_USERFUNC_INIT
    if (e)
    {
        array[0] = e->Name[0] ? (CONST_STRPTR)e->Name : _(MSG_NONAME);
        array[1] = e->Path;
    }
    else
    {
        array[0] = _(MSG_NAME);
        array[1] = _(MSG_PATH);
    }
    return 0;
    AROS_USERFUNC_EXIT
}

AROS_UFH3S(IPTR, AccessDisplay,
           AROS_UFHA(struct Hook *, hook, A0),
           AROS_UFHA(CONST_STRPTR *, array, A2),
           AROS_UFHA(APTR, entry, A1))
{
    AROS_USERFUNC_INIT
    struct EfsEditor_DATA *data = hook->h_Data;
    ULONG i = (ULONG)(IPTR)entry - 1;

    if (entry && data->cur && i < data->cur->NumAccess)
    {
        array[0] = data->cur->AccessName[i];
        array[1] = data->cur->AccessGroup[i] ? _(MSG_GROUP) : _(MSG_USER);
    }
    else
    {
        array[0] = "";
        array[1] = "";
    }
    return 0;
    AROS_USERFUNC_EXIT
}

/*** moving data between the gadgets and the current export ***************/

static void FillAccessList(struct EfsEditor_DATA *data)
{
    ULONG i;

    SET(data->acclist, MUIA_List_Quiet, TRUE);
    DoMethod(data->acclist, MUIM_List_Clear);
    if (data->cur)
        for (i = 0; i < data->cur->NumAccess; i++)
            DoMethod(data->acclist, MUIM_List_InsertSingle, (IPTR)(i + 1), MUIV_List_Insert_Bottom);
    SET(data->acclist, MUIA_List_Quiet, FALSE);
}

static void ShowExport(struct EfsEditor_DATA *data)
{
    struct Export *e = data->cur;
    ULONG sec = 0;

    SET(data->detail, MUIA_Disabled, e == NULL);
    SET(data->remove, MUIA_Disabled, e == NULL);
    if (e)
    {
        if (e->Flags & EXPF_NOSECURITY)
            sec = 2;
        else if (e->Flags & EXPF_FULLSECURITY)
            sec = 1;
        NNSET(data->name, MUIA_String_Contents, (IPTR)e->Name);
        NNSET(data->path, MUIA_String_Contents, (IPTR)e->Path);
        NNSET(data->security, MUIA_Cycle_Active, sec);
        NNSET(data->readonly, MUIA_Selected, (e->Flags & EXPF_READONLY) != 0);
        NNSET(data->removable, MUIA_Selected, (e->Flags & EXPF_REMOVABLE) != 0);
        NNSET(data->exall, MUIA_Selected, (e->Flags & EXPF_EMULATEEXALL) != 0);
        NNSET(data->snapshot, MUIA_Selected, (e->Flags & EXPF_SNAPSHOT) != 0);
        NNSET(data->leftout, MUIA_Selected, (e->Flags & EXPF_LEFTOUT) != 0);
        /* a nameless export is always a removable medium */
        SET(data->removable, MUIA_Disabled, !e->Name[0] || e->Name[0] == ':');
    }
    else
    {
        NNSET(data->name, MUIA_String_Contents, (IPTR)"");
        NNSET(data->path, MUIA_String_Contents, (IPTR)"");
    }
    FillAccessList(data);
    SET(data->accremove, MUIA_Disabled, TRUE);
}

static void StoreExport(struct EfsEditor_DATA *data)
{
    struct Export *e = data->cur;
    ULONG flags;

    if (!e)
        return;
    strncpy(e->Name, (char *)XGET(data->name, MUIA_String_Contents), EXP_NAMELEN - 1);
    e->Name[EXP_NAMELEN - 1] = '\0';
    strncpy(e->Path, (char *)XGET(data->path, MUIA_String_Contents), EXP_NAMELEN - 1);
    e->Path[EXP_NAMELEN - 1] = '\0';

    /* bits of VOLM flags this editor does not show are kept */
    flags = e->Flags & ~(EXPF_SNAPSHOT | EXPF_LEFTOUT | EXPF_FULLSECURITY | EXPF_NOSECURITY |
                         EXPF_EMULATEEXALL | EXPF_READONLY | EXPF_REMOVABLE);
    switch (XGET(data->security, MUIA_Cycle_Active))
    {
    case 1: flags |= EXPF_FULLSECURITY; break;
    case 2: flags |= EXPF_NOSECURITY; break;
    }
    if (XGET(data->readonly, MUIA_Selected))  flags |= EXPF_READONLY;
    if (XGET(data->removable, MUIA_Selected)) flags |= EXPF_REMOVABLE;
    if (XGET(data->exall, MUIA_Selected))     flags |= EXPF_EMULATEEXALL;
    if (XGET(data->snapshot, MUIA_Selected))  flags |= EXPF_SNAPSHOT;
    if (XGET(data->leftout, MUIA_Selected))   flags |= EXPF_LEFTOUT;
    e->Flags = flags;
    EfsPrefs_Normalise(e);

    NNSET(data->removable, MUIA_Selected, (e->Flags & EXPF_REMOVABLE) != 0);
    SET(data->removable, MUIA_Disabled, !e->Name[0] || e->Name[0] == ':');
    DoMethod(data->list, MUIM_List_Redraw, MUIV_List_Redraw_Active);
}

static void FillExportList(struct EfsEditor_DATA *data)
{
    struct Export *e;

    data->cur = NULL;
    SET(data->list, MUIA_List_Quiet, TRUE);
    DoMethod(data->list, MUIM_List_Clear);
    ForeachNode(&Exports, e)
    {
        ResolveNames(e);
        DoMethod(data->list, MUIM_List_InsertSingle, (IPTR)e, MUIV_List_Insert_Bottom);
    }
    SET(data->list, MUIA_List_Quiet, FALSE);
    if (!IsListEmpty((struct List *)&Exports))
        SET(data->list, MUIA_List_Active, MUIV_List_Active_Top);
    else
        ShowExport(data);
}

/*** methods ***************************************************************/

#define NOTIFY_CHANGE(obj, attr) \
    DoMethod(obj, MUIM_Notify, attr, MUIV_EveryTime, (IPTR)self, 1, MUIM_EfsEditor_Store)

Object *EfsEditor__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    Object *list, *add, *remove, *detail, *name, *path, *pathstr, *security;
    Object *readonly, *removable, *exall, *snapshot, *leftout, *acclist, *accadd, *accremove;
    static CONST_STRPTR seclabels[4];

    seclabels[0] = _(MSG_SEC_MOUNTS);
    seclabels[1] = _(MSG_SEC_MOUNTSFILES);
    seclabels[2] = _(MSG_SEC_NONE);
    seclabels[3] = NULL;

    self = (Object *)DoSuperNewTags
    (
        CLASS, self, NULL,
        MUIA_PrefsEditor_Name, __(MSG_WINTITLE),
        MUIA_PrefsEditor_Path, (IPTR)EFS_PREFS_PATH,
        MUIA_PrefsEditor_IconTool, (IPTR)"SYS:Prefs/Envoy/FilesystemExports",
        Child, (IPTR)HGroup,
            Child, (IPTR)VGroup, GroupFrameT(_(MSG_EXPORTS)), MUIA_HorizWeight, 80,
                Child, (IPTR)ListviewObject,
                    MUIA_CycleChain, 1,
                    MUIA_Listview_List, (IPTR)(list = (Object *)ListObject,
                        InputListFrame,
                        MUIA_List_Format, (IPTR)"MINWIDTH=60 BAR,MINWIDTH=80",
                        MUIA_List_Title, TRUE,
                        MUIA_List_AdjustWidth, TRUE,
                    End),
                End,
                Child, (IPTR)HGroup, MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(add = SimpleButton(_(MSG_ADD))),
                    Child, (IPTR)(remove = SimpleButton(_(MSG_REMOVE))),
                End,
            End,
            Child, (IPTR)(detail = (Object *)VGroup, MUIA_HorizWeight, 100,
                Child, (IPTR)ColGroup(2),
                    Child, (IPTR)Label2(_(MSG_NAME)),
                    Child, (IPTR)(name = (Object *)StringObject, StringFrame,
                        MUIA_String_MaxLen, EXP_NAMELEN, MUIA_CycleChain, 1, End),
                    Child, (IPTR)Label2(_(MSG_PATH)),
                    Child, (IPTR)(path = (Object *)PopaslObject,
                        MUIA_Popstring_String, (IPTR)(pathstr = (Object *)StringObject, StringFrame,
                            MUIA_String_MaxLen, EXP_NAMELEN, MUIA_CycleChain, 1, End),
                        MUIA_Popstring_Button, (IPTR)PopButton(MUII_PopDrawer),
                        ASLFR_DrawersOnly, TRUE,
                    End),
                    Child, (IPTR)Label1(_(MSG_SECURITY)),
                    Child, (IPTR)(security = (Object *)CycleObject,
                        MUIA_Cycle_Entries, (IPTR)seclabels,
                        MUIA_ShortHelp, __(MSG_SECHELP),
                        MUIA_CycleChain, 1,
                    End),
                End,
                Child, (IPTR)ColGroup(4),
                    Child, (IPTR)Label1(_(MSG_READONLY)),
                    Child, (IPTR)(readonly = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL)),
                    Child, (IPTR)Label1(_(MSG_REMOVABLE)),
                    Child, (IPTR)(removable = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL)),
                    Child, (IPTR)Label1(_(MSG_EMULATEEXALL)),
                    Child, (IPTR)(exall = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL)),
                    Child, (IPTR)Label1(_(MSG_SNAPSHOT)),
                    Child, (IPTR)(snapshot = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL)),
                    Child, (IPTR)Label1(_(MSG_LEFTOUT)),
                    Child, (IPTR)(leftout = MUI_MakeObject(MUIO_Checkmark, (IPTR)NULL)),
                    Child, (IPTR)HSpace(0),
                    Child, (IPTR)HSpace(0),
                End,
                Child, (IPTR)VGroup, GroupFrameT(_(MSG_ACCESS)),
                    Child, (IPTR)ListviewObject,
                        MUIA_CycleChain, 1,
                        MUIA_Listview_List, (IPTR)(acclist = (Object *)ListObject,
                            InputListFrame,
                            MUIA_List_Format, (IPTR)"BAR,",
                        End),
                    End,
                    Child, (IPTR)HGroup, MUIA_Group_SameWidth, TRUE,
                        Child, (IPTR)(accadd = SimpleButton(_(MSG_ADDACCESS))),
                        Child, (IPTR)(accremove = SimpleButton(_(MSG_REMOVEACCESS))),
                    End,
                End,
            End),
        End,
        TAG_DONE
    );

    if (self)
    {
        SETUP_INST_DATA;

        data->list = list;   data->add = add;   data->remove = remove;
        data->detail = detail; data->name = name; data->path = pathstr;
        data->security = security;
        data->readonly = readonly; data->removable = removable; data->exall = exall;
        data->snapshot = snapshot; data->leftout = leftout;
        data->acclist = acclist; data->accadd = accadd; data->accremove = accremove;

        data->disphook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(ExportDisplay);
        data->accdisphook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(AccessDisplay);
        data->accdisphook.h_Data = data;
        SET(list, MUIA_List_DisplayHook, (IPTR)&data->disphook);
        SET(acclist, MUIA_List_DisplayHook, (IPTR)&data->accdisphook);
        if (!EnvoyBase)
            SET(accadd, MUIA_Disabled, TRUE);

        DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)self, 1, MUIM_EfsEditor_Select);
        DoMethod(add, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)self, 1, MUIM_EfsEditor_Add);
        DoMethod(remove, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)self, 1, MUIM_EfsEditor_Remove);
        DoMethod(accadd, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)self, 1, MUIM_EfsEditor_AddAccess);
        DoMethod(accremove, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)self, 1, MUIM_EfsEditor_RemoveAccess);
        DoMethod(acclist, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)accremove, 3,
                 MUIM_Set, MUIA_Disabled, FALSE);
        NOTIFY_CHANGE(name, MUIA_String_Contents);
        NOTIFY_CHANGE(pathstr, MUIA_String_Contents);
        NOTIFY_CHANGE(security, MUIA_Cycle_Active);
        NOTIFY_CHANGE(readonly, MUIA_Selected);
        NOTIFY_CHANGE(removable, MUIA_Selected);
        NOTIFY_CHANGE(exall, MUIA_Selected);
        NOTIFY_CHANGE(snapshot, MUIA_Selected);
        NOTIFY_CHANGE(leftout, MUIA_Selected);

        FillExportList(data);
    }
    return self;
}

IPTR EfsEditor__MUIM_EfsEditor_Select(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct Export *e = NULL;

    DoMethod(data->list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&e);
    data->cur = e;
    ShowExport(data);
    return 0;
}

IPTR EfsEditor__MUIM_EfsEditor_Store(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    if (data->cur)
    {
        StoreExport(data);
        SET(self, MUIA_PrefsEditor_Changed, TRUE);
    }
    return 0;
}

IPTR EfsEditor__MUIM_EfsEditor_Add(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct Export *e = EfsPrefs_New();

    if (!e)
        return 0;
    strcpy(e->Name, _(MSG_NEWEXPORT));
    e->Flags = EXPF_FULLSECURITY;               /* the original editor's default */
    AddTail((struct List *)&Exports, (struct Node *)e);
    DoMethod(data->list, MUIM_List_InsertSingle, (IPTR)e, MUIV_List_Insert_Bottom);
    SET(data->list, MUIA_List_Active, MUIV_List_Active_Bottom);
    SET(_win(self), MUIA_Window_ActiveObject, (IPTR)data->name);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

IPTR EfsEditor__MUIM_EfsEditor_Remove(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct Export *e = data->cur;

    if (!e)
        return 0;
    data->cur = NULL;
    DoMethod(data->list, MUIM_List_Remove, MUIV_List_Remove_Active);
    Remove((struct Node *)e);
    FreeVec(e);
    DoMethod(self, MUIM_EfsEditor_Select);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

IPTR EfsEditor__MUIM_EfsEditor_AddAccess(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct Export *e = data->cur;
    char user[32], group[32];
    struct Window *win = NULL;
    UWORD id = 0;
    UBYTE isgroup = 0;
    BOOL found = FALSE;
    ULONG i;

    if (!e || !EnvoyBase || e->NumAccess >= EXP_MAXACCESS)
        return 0;
    if (_win(self))
        win = (struct Window *)XGET(_win(self), MUIA_Window_Window);
    user[0] = group[0] = '\0';
    if (!UserRequest(UGREQ_UserBuff, (IPTR)user, UGREQ_UserBuffLen, sizeof(user),
                     UGREQ_GroupBuff, (IPTR)group, UGREQ_GroupBuffLen, sizeof(group),
                     UGREQ_Title, __(MSG_PICKUSER),
                     win ? UGREQ_Window : TAG_IGNORE, (IPTR)win,
                     TAG_DONE))
        return 0;
    if (!AccountsBase)
        return 0;
    if (user[0])
    {
        struct UserInfo *ui = AllocUserInfo();
        if (ui)
        {
            if (NameToUser((STRPTR)user, ui) == 0)
            {
                id = ui->ui_UserID;
                found = TRUE;
            }
            FreeUserInfo(ui);
        }
    }
    else if (group[0])
    {
        struct GroupInfo *gi = AllocGroupInfo();
        if (gi)
        {
            if (NameToGroup((STRPTR)group, gi) == 0)
            {
                id = gi->gi_GroupID;
                isgroup = 1;
                found = TRUE;
            }
            FreeGroupInfo(gi);
        }
    }
    if (!found)
        return 0;
    for (i = 0; i < e->NumAccess; i++)
        if (e->AccessID[i] == id && e->AccessGroup[i] == isgroup)
            return 0;                           /* already in the list */
    e->AccessID[e->NumAccess] = id;
    e->AccessGroup[e->NumAccess] = isgroup;
    e->NumAccess++;
    ResolveNames(e);
    FillAccessList(data);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

IPTR EfsEditor__MUIM_EfsEditor_RemoveAccess(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    struct Export *e = data->cur;
    LONG active = XGET(data->acclist, MUIA_List_Active);
    ULONG i;

    if (!e || active < 0 || (ULONG)active >= e->NumAccess)
        return 0;
    for (i = active; i + 1 < e->NumAccess; i++)
    {
        e->AccessID[i] = e->AccessID[i + 1];
        e->AccessGroup[i] = e->AccessGroup[i + 1];
        memcpy(e->AccessName[i], e->AccessName[i + 1], sizeof(e->AccessName[i]));
    }
    e->NumAccess--;
    FillAccessList(data);
    SET(data->accremove, MUIA_Disabled, TRUE);
    SET(self, MUIA_PrefsEditor_Changed, TRUE);
    return 0;
}

IPTR EfsEditor__MUIM_PrefsEditor_ImportFH(Class *CLASS, Object *self, struct MUIP_PrefsEditor_ImportFH *message)
{
    SETUP_INST_DATA;
    BOOL ok = EfsPrefs_ImportFH(message->fh);

    if (ok)
        FillExportList(data);
    return ok;
}

IPTR EfsEditor__MUIM_PrefsEditor_ExportFH(Class *CLASS, Object *self, struct MUIP_PrefsEditor_ExportFH *message)
{
    SETUP_INST_DATA;

    StoreExport(data);
    return EfsPrefs_ExportFH(message->fh);
}

IPTR EfsEditor__MUIM_PrefsEditor_SetDefaults(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;

    data->cur = NULL;
    DoMethod(data->list, MUIM_List_Clear);
    EfsPrefs_Free();
    FillExportList(data);
    return TRUE;
}

ZUNE_CUSTOMCLASS_10
(
    EfsEditor, NULL, MUIC_PrefsEditor, NULL,
    OM_NEW,                         struct opSet *,
    MUIM_EfsEditor_Select,          Msg,
    MUIM_EfsEditor_Store,           Msg,
    MUIM_EfsEditor_Add,             Msg,
    MUIM_EfsEditor_Remove,          Msg,
    MUIM_EfsEditor_AddAccess,       Msg,
    MUIM_EfsEditor_RemoveAccess,    Msg,
    MUIM_PrefsEditor_ImportFH,      struct MUIP_PrefsEditor_ImportFH *,
    MUIM_PrefsEditor_ExportFH,      struct MUIP_PrefsEditor_ExportFH *,
    MUIM_PrefsEditor_SetDefaults,   Msg
);
