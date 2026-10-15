/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts - the Zune user interface
*/

#include <libraries/mui.h>
#include <libraries/asl.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <sys/errno.h>
#include <stdio.h>
#include <string.h>

#include "accounts.h"
#include "gui.h"
#include "locale.h"

enum
{
    ID_ADD = 1, ID_EDIT, ID_PASSWORD, ID_LOCK, ID_DELETE, ID_MYPW, ID_LOGIN, ID_REFRESH, ID_SELECT,
    ID_EDIT_SAVE, ID_EDIT_CANCEL, ID_PW_SAVE, ID_PW_CANCEL, ID_MYPW_SAVE, ID_MYPW_CANCEL
};

static struct AccountsState *st;

static Object *app, *win, *status, *list;
static Object *bt_add, *bt_edit, *bt_pw, *bt_lock, *bt_del, *bt_mypw, *bt_login, *bt_refresh;
static Object *ewin, *e_name, *e_real, *e_uid, *e_group, *e_home, *e_shell, *e_pw, *e_pw2, *e_hint;
static Object *pwin, *p_pw, *p_pw2;
static Object *mwin, *m_old, *m_pw, *m_pw2;
static Object *e_save, *e_cancel, *p_save, *p_cancel, *m_save, *m_cancel;

static STRPTR *groupnames;          /* cycle entries */
static struct Account *editing;     /* NULL: a new account */
static struct Account *pwtarget;

/* ------------------------------------------------------------------------ */

AROS_UFH3(static LONG, DisplayFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(STRPTR *, array, A2),
    AROS_UFHA(struct Account *, a, A1))
{
    AROS_USERFUNC_INIT

    static char uid[16];
    CONST_STRPTR g;

    if (!a)
    {
        array[0] = (STRPTR)MSG(MSG_COL_NAME);
        array[1] = (STRPTR)MSG(MSG_COL_UID);
        array[2] = (STRPTR)MSG(MSG_COL_GROUP);
        array[3] = (STRPTR)MSG(MSG_COL_REALNAME);
        array[4] = (STRPTR)MSG(MSG_COL_HOME);
        array[5] = (STRPTR)MSG(MSG_COL_STATUS);
        return 0;
    }
    snprintf(uid, sizeof(uid), "%ld", (long)a->uid);
    g = Acc_GroupName(st, a->gid);
    array[0] = a->Name;
    array[1] = uid;
    array[2] = (STRPTR)(g ? g : (CONST_STRPTR)"?");
    array[3] = a->RealName;
    array[4] = a->Home;
    array[5] = (STRPTR)(a->Locked ? MSG(MSG_STATUS_LOCKED) : (a->NoPassword ? MSG(MSG_STATUS_NOPASSWORD) : (CONST_STRPTR)""));
    return 0;

    AROS_USERFUNC_EXIT
}

static struct Hook displayhook = { { NULL, NULL }, (HOOKFUNC)DisplayFunc, NULL, NULL };

static void Message(CONST_STRPTR text)
{
    MUI_RequestA(app, win, 0, (STRPTR)MSG(MSG_TITLE), (STRPTR)MSG(MSG_REQ_OK), (STRPTR)text, NULL);
}

static void ErrorMessage(LONG err)
{
    char buf[128];
    ULONG id;

    switch (err)
    {
    case EPERM:     id = MSG_ERR_PERM;  break;
    case ENOENT:    id = MSG_ERR_NOENT; break;
    case EEXIST:    id = MSG_ERR_EXIST; break;
    case EINVAL:    id = MSG_ERR_INVAL; break;
    case EIO:       id = MSG_ERR_IO;    break;
    case ENOSYS:    id = MSG_ERR_NOSYS; break;
    default:
        snprintf(buf, sizeof(buf), MSG(MSG_ERR_OTHER), (long)err);
        Message(buf);
        return;
    }
    Message(MSG(id));
}

static struct Account *Selected(void)
{
    struct Account *a = NULL;

    DoMethod(list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&a);
    return a;
}

static void UpdateButtons(void)
{
    struct Account *a = Selected();
    BOOL sel = (a != NULL), admin = st->Admin;

    set(bt_add, MUIA_Disabled, !admin);
    set(bt_edit, MUIA_Disabled, !(admin && sel));
    set(bt_pw, MUIA_Disabled, !(admin && sel));
    set(bt_lock, MUIA_Disabled, !(admin && sel));
    set(bt_del, MUIA_Disabled, !(admin && sel));
    set(bt_lock, MUIA_Text_Contents, (IPTR)((sel && a->Locked) ? MSG(MSG_BUTTON_UNLOCK) : MSG(MSG_BUTTON_LOCK)));
    set(bt_login, MUIA_ShowMe, st->Multi && !admin);
    set(bt_mypw, MUIA_Disabled, st->Me[0] == '\0');
}

static void UpdateStatus(void)
{
    char buf[256];

    snprintf(buf, sizeof(buf), "%s%s%s", MSG(st->Multi ? MSG_MODE_SECURITY : MSG_MODE_SINGLE),
             st->Admin ? "" : "\n", st->Admin ? "" : (const char *)MSG(MSG_NOT_ADMIN));
    set(status, MUIA_Text_Contents, (IPTR)buf);
}

/* Reload the accounts; keep the selection by name */
static void Refresh(CONST_STRPTR select)
{
    char keep[ACC_NAMESIZE];
    ULONG i;

    keep[0] = '\0';
    if (select)
        strncpy(keep, select, sizeof(keep) - 1), keep[sizeof(keep) - 1] = '\0';

    set(list, MUIA_List_Quiet, TRUE);
    DoMethod(list, MUIM_List_Clear);
    Acc_Load(st);
    for (i = 0; i < st->NumAccounts; i++)
        DoMethod(list, MUIM_List_InsertSingle, (IPTR)&st->Accounts[i], MUIV_List_Insert_Bottom);
    set(list, MUIA_List_Quiet, FALSE);
    for (i = 0; keep[0] && i < st->NumAccounts; i++)
    {
        if (!strcmp(st->Accounts[i].Name, keep))
        {
            set(list, MUIA_List_Active, i);
            break;
        }
    }
    UpdateStatus();
    UpdateButtons();
}

/* ------------------------------------------------------------------------ */

static LONG GroupIndex(LONG gid)
{
    ULONG i;

    for (i = 0; i < st->NumGroups; i++)
        if (st->Groups[i].gid == gid)
            return i;
    return 0;
}

static void OpenEdit(struct Account *a)
{
    char uid[16];

    editing = a;
    set(ewin, MUIA_Window_Title, (IPTR)MSG(a ? MSG_EDIT_TITLE_EDIT : MSG_EDIT_TITLE_ADD));
    snprintf(uid, sizeof(uid), "%ld", (long)(a ? a->uid : Acc_NextFreeUID(st)));
    set(e_name, MUIA_String_Contents, (IPTR)(a ? a->Name : ""));
    set(e_real, MUIA_String_Contents, (IPTR)(a ? a->RealName : ""));
    set(e_uid, MUIA_String_Contents, (IPTR)uid);
    set(e_home, MUIA_String_Contents, (IPTR)(a ? a->Home : ""));
    set(e_shell, MUIA_String_Contents, (IPTR)(a ? a->Shell : ""));
    set(e_pw, MUIA_String_Contents, (IPTR)"");
    set(e_pw2, MUIA_String_Contents, (IPTR)"");
    set(e_hint, MUIA_ShowMe, a != NULL);
    if (st->NumGroups)
    {
        LONG idx = 0;
        if (a)
            idx = GroupIndex(a->gid);
        else
        {
            ULONG i;
            for (i = 0; i < st->NumGroups; i++)
                if (!Stricmp(st->Groups[i].Name, "users"))
                    idx = i;
        }
        set(e_group, MUIA_Cycle_Active, idx);
    }
    set(ewin, MUIA_Window_Open, TRUE);
    set(ewin, MUIA_Window_ActiveObject, (IPTR)e_name);
}

static BOOL NameOK(CONST_STRPTR s)
{
    if (!s || !s[0])
        return FALSE;
    for (; *s; s++)
        if (*s == ' ' || *s == ':' || *s == '|' || (UBYTE)*s < 0x20)
            return FALSE;
    return TRUE;
}

static void SaveEdit(void)
{
    STRPTR name = (STRPTR)XGET(e_name, MUIA_String_Contents);
    STRPTR real = (STRPTR)XGET(e_real, MUIA_String_Contents);
    STRPTR home = (STRPTR)XGET(e_home, MUIA_String_Contents);
    STRPTR shell = (STRPTR)XGET(e_shell, MUIA_String_Contents);
    STRPTR pw = (STRPTR)XGET(e_pw, MUIA_String_Contents);
    STRPTR pw2 = (STRPTR)XGET(e_pw2, MUIA_String_Contents);
    LONG uid = 0, gid = 0, err;
    char keep[ACC_NAMESIZE];

    if (!NameOK(name))
    {
        Message(MSG(MSG_ERR_NAME));
        return;
    }
    if (StrToLong((STRPTR)XGET(e_uid, MUIA_String_Contents), &uid) < 0 || uid < 0 || uid > 65535 ||
        (uid == 0 && !(editing && editing->uid == 0)))
    {
        Message(MSG(MSG_ERR_UID));
        return;
    }
    if (strcmp(pw, pw2))
    {
        Message(MSG(MSG_ERR_MISMATCH));
        return;
    }
    if (st->NumGroups)
        gid = st->Groups[XGET(e_group, MUIA_Cycle_Active)].gid;

    if (editing)
    {
        BOOL rename = strcmp(name, editing->Name) != 0;

        /* the stored hash depends on the name on a multi-user system */
        if (rename && st->Multi && !editing->NoPassword && !pw[0])
        {
            Message(MSG(MSG_ERR_RENAME_PASSWORD));
            return;
        }
        err = Acc_Modify(st, editing->Name, rename ? name : NULL, uid, gid, real, home, shell, pw[0] ? pw : NULL);
    }
    else
        err = Acc_Add(st, name, uid, gid, real, home, shell, pw);

    set(e_pw, MUIA_String_Contents, (IPTR)"");
    set(e_pw2, MUIA_String_Contents, (IPTR)"");
    if (err)
    {
        ErrorMessage(err);
        return;
    }
    strncpy(keep, name, sizeof(keep) - 1);
    keep[sizeof(keep) - 1] = '\0';
    set(ewin, MUIA_Window_Open, FALSE);
    Refresh(keep);
}

static void SavePassword(void)
{
    STRPTR pw = (STRPTR)XGET(p_pw, MUIA_String_Contents);
    STRPTR pw2 = (STRPTR)XGET(p_pw2, MUIA_String_Contents);
    LONG err;

    if (strcmp(pw, pw2))
    {
        Message(MSG(MSG_ERR_MISMATCH));
        return;
    }
    err = Acc_SetPassword(st, pwtarget->Name, pw);
    set(p_pw, MUIA_String_Contents, (IPTR)"");
    set(p_pw2, MUIA_String_Contents, (IPTR)"");
    if (err)
    {
        ErrorMessage(err);
        return;
    }
    set(pwin, MUIA_Window_Open, FALSE);
    Message(MSG(MSG_DONE_PASSWORD));
    Refresh(pwtarget->Name);
}

static void SaveMyPassword(void)
{
    STRPTR old = (STRPTR)XGET(m_old, MUIA_String_Contents);
    STRPTR pw = (STRPTR)XGET(m_pw, MUIA_String_Contents);
    STRPTR pw2 = (STRPTR)XGET(m_pw2, MUIA_String_Contents);
    char why[128], text[256];
    BOOL ok;

    if (strcmp(pw, pw2))
    {
        Message(MSG(MSG_ERR_MISMATCH));
        return;
    }
    ok = Acc_ChangeMyPassword(st, old, pw, why, sizeof(why));
    set(m_old, MUIA_String_Contents, (IPTR)"");
    set(m_pw, MUIA_String_Contents, (IPTR)"");
    set(m_pw2, MUIA_String_Contents, (IPTR)"");
    if (!ok)
    {
        snprintf(text, sizeof(text), MSG(MSG_ERR_PAM), why[0] ? why : "-");
        Message(text);
        return;
    }
    set(mwin, MUIA_Window_Open, FALSE);
    Message(MSG(MSG_DONE_PASSWORD));
    Refresh(st->Me);
}

/* ------------------------------------------------------------------------ */

static Object *Label2Obj(ULONG id)
{
    return Label2((STRPTR)MSG(id));
}

static Object *Secret(void)
{
    return StringObject, StringFrame, MUIA_String_Secret, TRUE, MUIA_String_MaxLen, 32, End;
}

BOOL Gui_Run(struct AccountsState *state)
{
    ULONG sigs = 0, i;
    BOOL running = TRUE;

    st = state;

    if (!(groupnames = AllocVec((st->NumGroups + 2) * sizeof(STRPTR), MEMF_CLEAR)))
        return FALSE;
    for (i = 0; i < st->NumGroups; i++)
        groupnames[i] = st->Groups[i].Name;
    if (!st->NumGroups)
        groupnames[0] = "?";

    app = ApplicationObject,
        MUIA_Application_Title,       (IPTR)"Accounts",
        MUIA_Application_Version,     (IPTR)"$VER: Accounts 1.0 (15.10.2026)",
        MUIA_Application_Copyright,   (IPTR)"(C) 2026, The AROS Development Team",
        MUIA_Application_Description, (IPTR)MSG(MSG_DESCRIPTION),
        MUIA_Application_Base,        (IPTR)"ACCOUNTS",

        SubWindow, win = WindowObject,
            MUIA_Window_Title, (IPTR)MSG(MSG_TITLE),
            MUIA_Window_ID, MAKE_ID('A','C','C','T'),
            WindowContents, VGroup,
                Child, status = TextObject, TextFrame, MUIA_Background, MUII_TextBack, End,
                Child, ListviewObject,
                    MUIA_Listview_List, list = ListObject, InputListFrame,
                        MUIA_List_Format, (IPTR)"BAR,P=\33r BAR,BAR,BAR,BAR,",
                        MUIA_List_Title, TRUE,
                        MUIA_List_DisplayHook, (IPTR)&displayhook,
                    End,
                End,
                Child, HGroup,
                    Child, bt_add = SimpleButton(MSG(MSG_BUTTON_ADD)),
                    Child, bt_edit = SimpleButton(MSG(MSG_BUTTON_EDIT)),
                    Child, bt_pw = SimpleButton(MSG(MSG_BUTTON_PASSWORD)),
                    Child, bt_lock = SimpleButton(MSG(MSG_BUTTON_LOCK)),
                    Child, bt_del = SimpleButton(MSG(MSG_BUTTON_DELETE)),
                End,
                Child, HGroup,
                    Child, bt_mypw = SimpleButton(MSG(MSG_BUTTON_MYPASSWORD)),
                    Child, bt_login = SimpleButton(MSG(MSG_BUTTON_LOGIN)),
                    Child, HSpace(0),
                    Child, bt_refresh = SimpleButton(MSG(MSG_BUTTON_REFRESH)),
                End,
            End,
        End,

        SubWindow, ewin = WindowObject,
            MUIA_Window_Title, (IPTR)MSG(MSG_EDIT_TITLE_ADD),
            MUIA_Window_ID, MAKE_ID('A','C','E','D'),
            WindowContents, VGroup,
                Child, ColGroup(2),
                    Child, Label2Obj(MSG_LABEL_NAME),
                    Child, e_name = StringObject, StringFrame, MUIA_String_MaxLen, ACC_NAMESIZE - 1, End,
                    Child, Label2Obj(MSG_LABEL_REALNAME),
                    Child, e_real = StringObject, StringFrame, MUIA_String_MaxLen, ACC_TEXTSIZE - 1, End,
                    Child, Label2Obj(MSG_LABEL_UID),
                    Child, e_uid = StringObject, StringFrame, MUIA_String_Accept, (IPTR)"0123456789", MUIA_String_MaxLen, 6, End,
                    Child, Label1((STRPTR)MSG(MSG_LABEL_GROUP)),
                    Child, e_group = CycleObject, MUIA_Cycle_Entries, (IPTR)groupnames, End,
                    Child, Label2Obj(MSG_LABEL_HOME),
                    Child, PopaslObject,
                        MUIA_Popstring_String, e_home = StringObject, StringFrame, MUIA_String_MaxLen, ACC_TEXTSIZE - 1, End,
                        MUIA_Popstring_Button, PopButton(MUII_PopDrawer),
                        ASLFR_DrawersOnly, TRUE,
                    End,
                    Child, Label2Obj(MSG_LABEL_SHELL),
                    Child, e_shell = StringObject, StringFrame, MUIA_String_MaxLen, ACC_TEXTSIZE - 1, End,
                    Child, Label2Obj(MSG_LABEL_PASSWORD),
                    Child, e_pw = Secret(),
                    Child, Label2Obj(MSG_LABEL_RETYPE),
                    Child, e_pw2 = Secret(),
                End,
                Child, e_hint = TextObject, MUIA_Text_Contents, (IPTR)MSG(MSG_EDIT_PASSWORD_HINT), End,
                Child, HGroup,
                    Child, e_save = SimpleButton(MSG(MSG_BUTTON_SAVE)),
                    Child, HSpace(0),
                    Child, e_cancel = SimpleButton(MSG(MSG_BUTTON_CANCEL)),
                End,
            End,
        End,

        SubWindow, pwin = WindowObject,
            MUIA_Window_Title, (IPTR)MSG(MSG_PASSWORD_TITLE),
            MUIA_Window_ID, MAKE_ID('A','C','P','W'),
            WindowContents, VGroup,
                Child, ColGroup(2),
                    Child, Label2Obj(MSG_LABEL_NEWPASSWORD),
                    Child, p_pw = Secret(),
                    Child, Label2Obj(MSG_LABEL_RETYPE),
                    Child, p_pw2 = Secret(),
                End,
                Child, HGroup,
                    Child, p_save = SimpleButton(MSG(MSG_BUTTON_SAVE)),
                    Child, HSpace(0),
                    Child, p_cancel = SimpleButton(MSG(MSG_BUTTON_CANCEL)),
                End,
            End,
        End,

        SubWindow, mwin = WindowObject,
            MUIA_Window_Title, (IPTR)MSG(MSG_MYPASSWORD_TITLE),
            MUIA_Window_ID, MAKE_ID('A','C','M','P'),
            WindowContents, VGroup,
                Child, ColGroup(2),
                    Child, Label2Obj(MSG_LABEL_OLDPASSWORD),
                    Child, m_old = Secret(),
                    Child, Label2Obj(MSG_LABEL_NEWPASSWORD),
                    Child, m_pw = Secret(),
                    Child, Label2Obj(MSG_LABEL_RETYPE),
                    Child, m_pw2 = Secret(),
                End,
                Child, HGroup,
                    Child, m_save = SimpleButton(MSG(MSG_BUTTON_SAVE)),
                    Child, HSpace(0),
                    Child, m_cancel = SimpleButton(MSG(MSG_BUTTON_CANCEL)),
                End,
            End,
        End,
    End;

    if (!app)
    {
        FreeVec(groupnames);
        return FALSE;
    }

    {
        struct { Object *w, *save, *cancel; ULONG sid, cid; } dlg[] =
        {
            { ewin, e_save, e_cancel, ID_EDIT_SAVE, ID_EDIT_CANCEL },
            { pwin, p_save, p_cancel, ID_PW_SAVE,   ID_PW_CANCEL   },
            { mwin, m_save, m_cancel, ID_MYPW_SAVE, ID_MYPW_CANCEL }
        };
        for (i = 0; i < 3; i++)
        {
            DoMethod(dlg[i].save, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, dlg[i].sid);
            DoMethod(dlg[i].cancel, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, dlg[i].cid);
            DoMethod(dlg[i].w, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2, MUIM_Application_ReturnID, dlg[i].cid);
        }
    }

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(bt_add, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_ADD);
    DoMethod(bt_edit, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_EDIT);
    DoMethod(bt_pw, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_PASSWORD);
    DoMethod(bt_lock, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_LOCK);
    DoMethod(bt_del, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_DELETE);
    DoMethod(bt_mypw, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_MYPW);
    DoMethod(bt_login, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_LOGIN);
    DoMethod(bt_refresh, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_REFRESH);
    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, ID_SELECT);
    DoMethod(list, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_EDIT);

    Refresh(NULL);
    set(win, MUIA_Window_Open, TRUE);

    while (running)
    {
        ULONG id = DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);
        struct Account *a = Selected();

        switch (id)
        {
        case MUIV_Application_ReturnID_Quit:
            running = FALSE;
            break;
        case ID_SELECT:
            UpdateButtons();
            break;
        case ID_ADD:
            if (st->Admin)
                OpenEdit(NULL);
            break;
        case ID_EDIT:
            if (st->Admin && a)
                OpenEdit(a);
            break;
        case ID_PASSWORD:
            if (st->Admin && a)
            {
                pwtarget = a;
                set(pwin, MUIA_Window_Open, TRUE);
                set(pwin, MUIA_Window_ActiveObject, (IPTR)p_pw);
            }
            break;
        case ID_LOCK:
            if (st->Admin && a)
            {
                char keep[ACC_NAMESIZE];
                LONG err;

                strncpy(keep, a->Name, sizeof(keep) - 1);
                keep[sizeof(keep) - 1] = '\0';
                if ((err = Acc_Lock(st, a->Name, !a->Locked)))
                    ErrorMessage(err);
                Refresh(keep);
            }
            break;
        case ID_DELETE:
            if (st->Admin && a)
            {
                char text[256];
                LONG err;

                snprintf(text, sizeof(text), MSG(MSG_REQ_DELETE), a->Name);
                if (MUI_RequestA(app, win, 0, (STRPTR)MSG(MSG_TITLE), (STRPTR)MSG(MSG_REQ_YESNO), text, NULL) == 1)
                {
                    if ((err = Acc_Delete(st, a->Name)))
                        ErrorMessage(err);
                    Refresh(NULL);
                }
            }
            break;
        case ID_MYPW:
            set(mwin, MUIA_Window_Open, TRUE);
            set(mwin, MUIA_Window_ActiveObject, (IPTR)m_old);
            break;
        case ID_LOGIN:
            if (Acc_LoginRoot(st))
                Refresh(a ? a->Name : NULL);
            break;
        case ID_REFRESH:
            Refresh(a ? a->Name : NULL);
            break;
        case ID_EDIT_SAVE:
            SaveEdit();
            break;
        case ID_EDIT_CANCEL:
            set(e_pw, MUIA_String_Contents, (IPTR)"");
            set(e_pw2, MUIA_String_Contents, (IPTR)"");
            set(ewin, MUIA_Window_Open, FALSE);
            break;
        case ID_PW_SAVE:
            SavePassword();
            break;
        case ID_PW_CANCEL:
            set(p_pw, MUIA_String_Contents, (IPTR)"");
            set(p_pw2, MUIA_String_Contents, (IPTR)"");
            set(pwin, MUIA_Window_Open, FALSE);
            break;
        case ID_MYPW_SAVE:
            SaveMyPassword();
            break;
        case ID_MYPW_CANCEL:
            set(m_old, MUIA_String_Contents, (IPTR)"");
            set(m_pw, MUIA_String_Contents, (IPTR)"");
            set(m_pw2, MUIA_String_Contents, (IPTR)"");
            set(mwin, MUIA_Window_Open, FALSE);
            break;
        }
        if (running && sigs)
        {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C);
            if (sigs & SIGBREAKF_CTRL_C)
                running = FALSE;
        }
    }

    MUI_DisposeObject(app);
    FreeVec(groupnames);
    return TRUE;
}
