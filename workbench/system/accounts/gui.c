/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Accounts - the Zune user interface (users and groups)
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
    ID_EDIT_SAVE, ID_EDIT_CANCEL, ID_PW_SAVE, ID_PW_CANCEL, ID_MYPW_SAVE, ID_MYPW_CANCEL,
    ID_GADD, ID_GEDIT, ID_GDELETE, ID_GSELECT, ID_GEDIT_SAVE, ID_GEDIT_CANCEL, ID_PW_REMOVE
};

static struct AccountsState *st;

static Object *app, *win, *status, *list, *glist;
static Object *bt_add, *bt_edit, *bt_pw, *bt_lock, *bt_del, *bt_mypw, *bt_login, *bt_refresh;
static Object *bt_gadd, *bt_gedit, *bt_gdel;
static Object *u_buttons, *u_mine, *g_buttons;     /* hidden when nothing may be changed */
static Object *umenu, *um_edit, *um_pw, *um_lock, *um_unlock, *um_del, *um_add;
static Object *gmenu, *gm_edit, *gm_del, *gm_add;
static struct MUI_CustomClass *listclass;
static Object *ewin, *e_name, *e_real, *e_uid, *e_group, *e_home, *e_shell, *e_pw, *e_pw2, *e_hint;
static Object *gwin, *g_name, *g_gid, *g_desc, *g_desclabel, *g_members, *g_primary;
static Object *pwin, *p_pw, *p_pw2, *p_who, *p_remove;
static Object *mwin, *m_old, *m_pw, *m_pw2;
static Object *e_save, *e_cancel, *g_save, *g_cancel, *p_save, *p_cancel, *m_save, *m_cancel;

static STRPTR *groupnames;          /* cycle entries */
static struct Account *editing;     /* NULL: a new account */
static char pwname[ACC_NAMESIZE];  /* the account in the password window */
static char geditname[ACC_NAMESIZE];/* the group being edited, "" for a new one */
static LONG geditgid;
static CONST_STRPTR pages[3];

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

/* Everybody in a group: through the primary group or listed */
static void MemberText(struct GroupEntry *g, STRPTR buf, ULONG size)
{
    ULONG i, len = 0;

    buf[0] = '\0';
    for (i = 0; i < st->NumAccounts; i++)
    {
        struct Account *a = &st->Accounts[i];
        ULONG n = strlen(a->Name);

        if (!Acc_InGroup(g, a))
            continue;
        if (len + n + 6 >= size)
        {
            strcpy(buf + len, len ? ", ..." : "...");
            return;
        }
        if (len)
        {
            strcpy(buf + len, ", ");
            len += 2;
        }
        strcpy(buf + len, a->Name);
        len += n;
    }
}

AROS_UFH3(static LONG, GroupDisplayFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(STRPTR *, array, A2),
    AROS_UFHA(struct GroupEntry *, g, A1))
{
    AROS_USERFUNC_INIT

    static char gid[16], members[ACC_TEXTSIZE];

    if (!g)
    {
        array[0] = (STRPTR)MSG(MSG_COL_GROUPNAME);
        array[1] = (STRPTR)MSG(MSG_COL_GID);
        array[2] = (STRPTR)MSG(MSG_COL_MEMBERS);
        array[3] = (STRPTR)MSG(MSG_COL_DESCRIPTION);
        return 0;
    }
    snprintf(gid, sizeof(gid), "%ld", (long)g->gid);
    MemberText(g, members, sizeof(members));
    array[0] = g->Name;
    array[1] = gid;
    array[2] = members;
    array[3] = g->Description;
    return 0;

    AROS_USERFUNC_EXIT
}

static struct Hook gdisplayhook = { { NULL, NULL }, (HOOKFUNC)GroupDisplayFunc, NULL, NULL };

static void Message(CONST_STRPTR text)
{
    MUI_RequestA(app, win, 0, (STRPTR)MSG(MSG_TITLE), (STRPTR)MSG(MSG_REQ_OK), (STRPTR)text, NULL);
}

/* group: the change was to a group */
static void ErrorMessage(LONG err, BOOL group)
{
    char buf[128];
    ULONG id;

    switch (err)
    {
    case EPERM:     id = group ? MSG_ERR_GROUP_PERM : MSG_ERR_PERM;   break;
    case ENOENT:    id = group ? MSG_ERR_GROUP_NOENT : MSG_ERR_NOENT; break;
    case EEXIST:    id = group ? MSG_ERR_GROUP_EXIST : MSG_ERR_EXIST; break;
    case EINVAL:    id = MSG_ERR_INVAL; break;
    case EIO:       id = group ? MSG_ERR_GROUP_IO : MSG_ERR_IO;       break;
    case EBUSY:     id = MSG_ERR_BUSY;  break;
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

static struct GroupEntry *SelectedGroup(void)
{
    struct GroupEntry *g = NULL;

    DoMethod(glist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&g);
    return g;
}

/* Without the right to change anything the lists are only shown: the
 * editing buttons are hidden and the lists have no context menus */
static void UpdateButtons(void)
{
    struct Account *a = Selected();
    BOOL sel = (a != NULL), edit = Acc_CanEdit(st), gsel = (SelectedGroup() != NULL);

    set(u_buttons, MUIA_ShowMe, edit);
    set(g_buttons, MUIA_ShowMe, edit);
    set(u_mine, MUIA_ShowMe, !st->ReadOnly && st->Me[0] != '\0');
    set(bt_login, MUIA_ShowMe, st->Multi && !st->Admin && !st->ReadOnly);

    set(bt_gedit, MUIA_Disabled, !gsel);
    set(bt_gdel, MUIA_Disabled, !gsel);
    set(bt_edit, MUIA_Disabled, !sel);
    set(bt_pw, MUIA_Disabled, !sel);
    set(bt_lock, MUIA_Disabled, !sel);
    set(bt_del, MUIA_Disabled, !sel);
    set(bt_lock, MUIA_Text_Contents, (IPTR)((sel && a->Locked) ? MSG(MSG_BUTTON_UNLOCK) : MSG(MSG_BUTTON_LOCK)));

    /* the context menus act on the entry under the pointer */
    set(um_edit, MUIA_Menuitem_Enabled, sel);
    set(um_pw, MUIA_Menuitem_Enabled, sel);
    set(um_lock, MUIA_Menuitem_Enabled, sel && !a->Locked);
    set(um_unlock, MUIA_Menuitem_Enabled, sel && a->Locked);
    set(um_del, MUIA_Menuitem_Enabled, sel);
    set(gm_edit, MUIA_Menuitem_Enabled, gsel);
    set(gm_del, MUIA_Menuitem_Enabled, gsel);
}

static void UpdateStatus(void)
{
    char buf[256];
    CONST_STRPTR why = st->ReadOnly ? MSG(MSG_READONLY_MEDIA) : (!st->Admin ? MSG(MSG_NOT_ADMIN) : NULL);

    snprintf(buf, sizeof(buf), "%s%s%s", MSG(st->Multi ? MSG_MODE_SECURITY : MSG_MODE_SINGLE),
             why ? "\n" : "", why ? (const char *)why : "");
    set(status, MUIA_Text_Contents, (IPTR)buf);
}

/*
 * The lists: a right click selects the entry under the pointer (or none)
 * and opens the list's context menu - only when something may be changed.
 */
BOOPSI_DISPATCHER(IPTR, ListDispatcher, cl, obj, msg)
{
    if (msg->MethodID == MUIM_ContextMenuBuild)
    {
        struct MUIP_ContextMenuBuild *m = (struct MUIP_ContextMenuBuild *)msg;
        struct MUI_List_TestPos_Result pos;

        if (!Acc_CanEdit(st))
            return 0;
        pos.entry = -1;
        DoMethod(obj, MUIM_List_TestPos, m->mx, m->my, (IPTR)&pos);
        set(obj, MUIA_List_Active, pos.entry >= 0 ? pos.entry : MUIV_List_Active_Off);
        UpdateButtons();
    }
    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

static Object *MenuItem(ULONG id)
{
    return MenuitemObject, MUIA_Menuitem_Title, (IPTR)MSG(id), End;
}

static Object *MenuBar(void)
{
    return MenuitemObject, MUIA_Menuitem_Title, (IPTR)NM_BARLABEL, End;
}

/* The entries of the primary group cycle, from st->Groups */
static BOOL MakeGroupNames(void)
{
    STRPTR *names;
    ULONG i;

    if (!(names = AllocVec((st->NumGroups + 2) * sizeof(STRPTR), MEMF_CLEAR)))
        return FALSE;
    for (i = 0; i < st->NumGroups; i++)
        names[i] = st->Groups[i].Name;
    if (!st->NumGroups)
        names[0] = "?";
    if (e_group)
        set(e_group, MUIA_Cycle_Entries, (IPTR)names);
    if (groupnames)
        FreeVec(groupnames);
    groupnames = names;
    return TRUE;
}

/* Reload the accounts and groups; keep the selections by name */
static void Refresh(CONST_STRPTR select, CONST_STRPTR gselect)
{
    char keep[ACC_NAMESIZE], gkeep[ACC_NAMESIZE];
    struct GroupEntry *g = SelectedGroup();
    ULONG i;

    keep[0] = gkeep[0] = '\0';
    if (select)
        strncpy(keep, select, sizeof(keep) - 1), keep[sizeof(keep) - 1] = '\0';
    if (gselect || g)
        strncpy(gkeep, gselect ? gselect : (CONST_STRPTR)g->Name, sizeof(gkeep) - 1), gkeep[sizeof(gkeep) - 1] = '\0';

    set(list, MUIA_List_Quiet, TRUE);
    set(glist, MUIA_List_Quiet, TRUE);
    DoMethod(list, MUIM_List_Clear);
    DoMethod(glist, MUIM_List_Clear);
    Acc_LoadGroups(st);
    MakeGroupNames();
    Acc_Load(st);
    for (i = 0; i < st->NumAccounts; i++)
        DoMethod(list, MUIM_List_InsertSingle, (IPTR)&st->Accounts[i], MUIV_List_Insert_Bottom);
    for (i = 0; i < st->NumGroups; i++)
        DoMethod(glist, MUIM_List_InsertSingle, (IPTR)&st->Groups[i], MUIV_List_Insert_Bottom);
    set(list, MUIA_List_Quiet, FALSE);
    set(glist, MUIA_List_Quiet, FALSE);
    for (i = 0; keep[0] && i < st->NumAccounts; i++)
    {
        if (!strcmp(st->Accounts[i].Name, keep))
        {
            set(list, MUIA_List_Active, i);
            break;
        }
    }
    for (i = 0; gkeep[0] && i < st->NumGroups; i++)
    {
        if (!strcmp(st->Groups[i].Name, gkeep))
        {
            set(glist, MUIA_List_Active, i);
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
        ErrorMessage(err, FALSE);
        return;
    }
    strncpy(keep, name, sizeof(keep) - 1);
    keep[sizeof(keep) - 1] = '\0';
    set(ewin, MUIA_Window_Open, FALSE);
    Refresh(keep, NULL);
}

static void OpenPassword(struct Account *a)
{
    char text[ACC_NAMESIZE + 64];

    strncpy(pwname, a->Name, sizeof(pwname) - 1);
    pwname[sizeof(pwname) - 1] = '\0';
    snprintf(text, sizeof(text), MSG(MSG_PASSWORD_FOR), a->Name);
    set(p_who, MUIA_Text_Contents, (IPTR)text);
    set(p_remove, MUIA_Disabled, a->NoPassword && !a->Locked);
    set(pwin, MUIA_Window_Open, TRUE);
    set(pwin, MUIA_Window_ActiveObject, (IPTR)p_pw);
}

static void ClosePassword(void)
{
    set(p_pw, MUIA_String_Contents, (IPTR)"");
    set(p_pw2, MUIA_String_Contents, (IPTR)"");
    set(pwin, MUIA_Window_Open, FALSE);
}

static void SavePassword(void)
{
    STRPTR pw = (STRPTR)XGET(p_pw, MUIA_String_Contents);
    STRPTR pw2 = (STRPTR)XGET(p_pw2, MUIA_String_Contents);
    LONG err;

    /* an empty password is removed with its own button, on purpose */
    if (!pw[0])
    {
        Message(MSG(MSG_ERR_EMPTY_PASSWORD));
        return;
    }
    if (strcmp(pw, pw2))
    {
        Message(MSG(MSG_ERR_MISMATCH));
        return;
    }
    err = Acc_SetPassword(st, pwname, pw);
    set(p_pw, MUIA_String_Contents, (IPTR)"");
    set(p_pw2, MUIA_String_Contents, (IPTR)"");
    if (err)
    {
        ErrorMessage(err, FALSE);
        return;
    }
    ClosePassword();
    Message(MSG(MSG_DONE_PASSWORD));
    Refresh(pwname, NULL);
}

static void RemovePassword(void)
{
    char text[ACC_NAMESIZE + 256];
    LONG err;

    snprintf(text, sizeof(text), MSG(MSG_REQ_REMOVE_PASSWORD), pwname);
    if (MUI_RequestA(app, pwin, 0, (STRPTR)MSG(MSG_TITLE), (STRPTR)MSG(MSG_REQ_REMOVE_BUTTONS), text, NULL) != 1)
        return;
    if ((err = Acc_SetPassword(st, pwname, "")))
    {
        ErrorMessage(err, FALSE);
        return;
    }
    ClosePassword();
    Message(MSG(MSG_DONE_REMOVED));
    Refresh(pwname, NULL);
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
    Refresh(st->Me, NULL);
}

/* ------------------------------------------------------------------------ */
/* Groups                                                                   */

static BOOL GroupNameOK(CONST_STRPTR s)
{
    if (!s || !s[0])
        return FALSE;
    for (; *s; s++)
        if (*s == ' ' || *s == ':' || *s == '|' || *s == ',' || (UBYTE)*s < 0x20)
            return FALSE;
    return TRUE;
}

static void OpenGroupEdit(struct GroupEntry *g)
{
    char gid[16], primary[ACC_TEXTSIZE];
    ULONG i, len = 0, n = 0;

    geditname[0] = '\0';
    geditgid = -1;
    if (g)
    {
        strncpy(geditname, g->Name, sizeof(geditname) - 1);
        geditname[sizeof(geditname) - 1] = '\0';
        geditgid = g->gid;
    }
    set(gwin, MUIA_Window_Title, (IPTR)MSG(g ? MSG_GROUP_TITLE_EDIT : MSG_GROUP_TITLE_ADD));
    snprintf(gid, sizeof(gid), "%ld", (long)(g ? g->gid : Acc_NextFreeGID(st)));
    set(g_name, MUIA_String_Contents, (IPTR)(g ? g->Name : ""));
    set(g_gid, MUIA_String_Contents, (IPTR)gid);
    set(g_desc, MUIA_String_Contents, (IPTR)(g ? g->Description : ""));

    /* the users whose primary group it is belong to it anyway: they are
     * named, not offered */
    primary[0] = '\0';
    set(g_members, MUIA_List_Quiet, TRUE);
    DoMethod(g_members, MUIM_List_Clear);
    for (i = 0; i < st->NumAccounts; i++)
    {
        struct Account *a = &st->Accounts[i];

        if (g && a->gid == g->gid)
        {
            ULONG l = strlen(a->Name);
            if (len + l + 6 < sizeof(primary))
            {
                if (len)
                    strcpy(primary + len, ", "), len += 2;
                strcpy(primary + len, a->Name);
                len += l;
            }
            else if (len + 5 < sizeof(primary))
                strcpy(primary + len, ", ..."), len = sizeof(primary);
            continue;
        }
        DoMethod(g_members, MUIM_List_InsertSingle, (IPTR)a->Name, MUIV_List_Insert_Bottom);
        if (g && Acc_Listed(g, a->Name))
            DoMethod(g_members, MUIM_List_Select, n, MUIV_List_Select_On, NULL);
        n++;
    }
    set(g_members, MUIA_List_Quiet, FALSE);
    {
        char text[ACC_TEXTSIZE + 128];
        snprintf(text, sizeof(text), MSG(MSG_GROUP_PRIMARY), primary[0] ? primary : "-");
        set(g_primary, MUIA_Text_Contents, (IPTR)text);
    }
    set(gwin, MUIA_Window_Open, TRUE);
    set(gwin, MUIA_Window_ActiveObject, (IPTR)g_name);
}

/* The selected members as a NULL-terminated array (one block; FreeVec()) */
static STRPTR *SelectedMembers(void)
{
    ULONG i, n = 0, entries = XGET(g_members, MUIA_List_Entries);
    STRPTR *m, name;
    LONG state;

    if (!(m = AllocVec((entries + 1) * sizeof(STRPTR), MEMF_CLEAR)))
        return NULL;
    for (i = 0; i < entries; i++)
    {
        state = MUIV_List_Select_Off;
        DoMethod(g_members, MUIM_List_Select, i, MUIV_List_Select_Ask, (IPTR)&state);
        if (state == MUIV_List_Select_On)
        {
            name = NULL;
            DoMethod(g_members, MUIM_List_GetEntry, i, (IPTR)&name);
            if (name)
                m[n++] = name;
        }
    }
    m[n] = NULL;
    return m;
}

static void SaveGroupEdit(void)
{
    STRPTR name = (STRPTR)XGET(g_name, MUIA_String_Contents);
    STRPTR desc = (STRPTR)XGET(g_desc, MUIA_String_Contents);
    STRPTR *members;
    LONG gid = 0, err;
    char keep[ACC_NAMESIZE];

    if (!GroupNameOK(name))
    {
        Message(MSG(MSG_ERR_GROUPNAME));
        return;
    }
    if (StrToLong((STRPTR)XGET(g_gid, MUIA_String_Contents), &gid) < 0 || gid < 0 || gid > 65535 ||
        (gid == 0 && geditgid != 0))
    {
        Message(MSG(MSG_ERR_GID));
        return;
    }
    if (!(members = SelectedMembers()))
    {
        ErrorMessage(ENOMEM, TRUE);
        return;
    }
    if (geditname[0])
        err = Acc_GroupModify(st, geditname, strcmp(name, geditname) ? name : NULL, gid, desc, members);
    else
        err = Acc_GroupAdd(st, name, gid, desc, members);
    FreeVec(members);
    if (err)
    {
        ErrorMessage(err, TRUE);
        return;
    }
    strncpy(keep, name, sizeof(keep) - 1);
    keep[sizeof(keep) - 1] = '\0';
    set(gwin, MUIA_Window_Open, FALSE);
    {
        struct Account *a = Selected();
        Refresh(a ? a->Name : NULL, keep);
    }
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

    if (!MakeGroupNames())
        return FALSE;
    pages[0] = MSG(MSG_PAGE_USERS);
    pages[1] = MSG(MSG_PAGE_GROUPS);
    pages[2] = NULL;

    if (!(listclass = MUI_CreateCustomClass(NULL, MUIC_List, NULL, 0, ListDispatcher)))
    {
        FreeVec(groupnames);
        return FALSE;
    }
    umenu = MenustripObject,
        MUIA_Family_Child, MenuObjectT((IPTR)MSG(MSG_MENU_ACCOUNT)),
            MUIA_Family_Child, um_edit = MenuItem(MSG_MENU_EDIT),
            MUIA_Family_Child, um_pw = MenuItem(MSG_MENU_PASSWORD),
            MUIA_Family_Child, um_lock = MenuItem(MSG_MENU_LOCK),
            MUIA_Family_Child, um_unlock = MenuItem(MSG_MENU_UNLOCK),
            MUIA_Family_Child, um_del = MenuItem(MSG_MENU_DELETE),
            MUIA_Family_Child, MenuBar(),
            MUIA_Family_Child, um_add = MenuItem(MSG_MENU_ADD_ACCOUNT),
        End,
    End;
    gmenu = MenustripObject,
        MUIA_Family_Child, MenuObjectT((IPTR)MSG(MSG_MENU_GROUP)),
            MUIA_Family_Child, gm_edit = MenuItem(MSG_MENU_EDIT),
            MUIA_Family_Child, gm_del = MenuItem(MSG_MENU_DELETE),
            MUIA_Family_Child, MenuBar(),
            MUIA_Family_Child, gm_add = MenuItem(MSG_MENU_ADD_GROUP),
        End,
    End;

    app = ApplicationObject,
        MUIA_Application_Title,       (IPTR)"Accounts",
        MUIA_Application_Version,     (IPTR)"$VER: Accounts 1.1 (15.10.2026)",
        MUIA_Application_Copyright,   (IPTR)"(C) 2026, The AROS Development Team",
        MUIA_Application_Description, (IPTR)MSG(MSG_DESCRIPTION),
        MUIA_Application_Base,        (IPTR)"ACCOUNTS",

        SubWindow, win = WindowObject,
            MUIA_Window_Title, (IPTR)MSG(MSG_TITLE),
            MUIA_Window_ID, MAKE_ID('A','C','C','T'),
            WindowContents, VGroup,
                Child, status = TextObject, TextFrame, MUIA_Background, MUII_TextBack, End,
                Child, RegisterGroup(pages),
                    MUIA_Register_Frame, TRUE,
                    Child, VGroup,
                        Child, ListviewObject,
                            MUIA_Listview_List, list = NewObject(listclass->mcc_Class, NULL, InputListFrame,
                                MUIA_ContextMenu, (IPTR)umenu,
                                MUIA_List_Format, (IPTR)"BAR,P=\33r BAR,BAR,BAR,BAR,",
                                MUIA_List_Title, TRUE,
                                MUIA_List_DisplayHook, (IPTR)&displayhook,
                            TAG_DONE),
                        End,
                        Child, u_buttons = HGroup,
                            Child, bt_add = SimpleButton(MSG(MSG_BUTTON_ADD)),
                            Child, bt_edit = SimpleButton(MSG(MSG_BUTTON_EDIT)),
                            Child, bt_pw = SimpleButton(MSG(MSG_BUTTON_PASSWORD)),
                            Child, bt_lock = SimpleButton(MSG(MSG_BUTTON_LOCK)),
                            Child, bt_del = SimpleButton(MSG(MSG_BUTTON_DELETE)),
                        End,
                        Child, u_mine = HGroup,
                            Child, bt_mypw = SimpleButton(MSG(MSG_BUTTON_MYPASSWORD)),
                            Child, HSpace(0),
                        End,
                    End,
                    Child, VGroup,
                        Child, ListviewObject,
                            MUIA_Listview_List, glist = NewObject(listclass->mcc_Class, NULL, InputListFrame,
                                MUIA_ContextMenu, (IPTR)gmenu,
                                MUIA_List_Format, (IPTR)"BAR,P=\33r BAR,BAR,",
                                MUIA_List_Title, TRUE,
                                MUIA_List_DisplayHook, (IPTR)&gdisplayhook,
                            TAG_DONE),
                        End,
                        Child, g_buttons = HGroup,
                            Child, bt_gadd = SimpleButton(MSG(MSG_BUTTON_GADD)),
                            Child, bt_gedit = SimpleButton(MSG(MSG_BUTTON_GEDIT)),
                            Child, bt_gdel = SimpleButton(MSG(MSG_BUTTON_GDELETE)),
                            Child, HSpace(0),
                        End,
                    End,
                End,
                Child, HGroup,
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

        SubWindow, gwin = WindowObject,
            MUIA_Window_Title, (IPTR)MSG(MSG_GROUP_TITLE_ADD),
            MUIA_Window_ID, MAKE_ID('A','C','G','R'),
            WindowContents, VGroup,
                Child, ColGroup(2),
                    Child, Label2Obj(MSG_LABEL_GROUPNAME),
                    Child, g_name = StringObject, StringFrame, MUIA_String_MaxLen, ACC_NAMESIZE - 1, End,
                    Child, Label2Obj(MSG_LABEL_GID),
                    Child, g_gid = StringObject, StringFrame, MUIA_String_Accept, (IPTR)"0123456789", MUIA_String_MaxLen, 6, End,
                    Child, g_desclabel = Label2Obj(MSG_LABEL_DESCRIPTION),
                    Child, g_desc = StringObject, StringFrame, MUIA_String_MaxLen, ACC_TEXTSIZE - 1, End,
                End,
                Child, VGroup, GroupFrameT(MSG(MSG_LABEL_MEMBERS)),
                    Child, ListviewObject,
                        MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_Always,
                        MUIA_Listview_List, g_members = ListObject, InputListFrame,
                            MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                            MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                        End,
                    End,
                    Child, g_primary = TextObject, MUIA_Text_Contents, (IPTR)"", End,
                End,
                Child, HGroup,
                    Child, g_save = SimpleButton(MSG(MSG_BUTTON_SAVE)),
                    Child, HSpace(0),
                    Child, g_cancel = SimpleButton(MSG(MSG_BUTTON_CANCEL)),
                End,
            End,
        End,

        SubWindow, pwin = WindowObject,
            MUIA_Window_Title, (IPTR)MSG(MSG_PASSWORD_TITLE),
            MUIA_Window_ID, MAKE_ID('A','C','P','W'),
            WindowContents, VGroup,
                Child, p_who = TextObject, MUIA_Text_Contents, (IPTR)"", End,
                Child, ColGroup(2),
                    Child, Label2Obj(MSG_LABEL_NEWPASSWORD),
                    Child, p_pw = Secret(),
                    Child, Label2Obj(MSG_LABEL_RETYPE),
                    Child, p_pw2 = Secret(),
                End,
                Child, HGroup,
                    Child, p_save = SimpleButton(MSG(MSG_BUTTON_SAVE)),
                    Child, p_remove = SimpleButton(MSG(MSG_BUTTON_REMOVEPW)),
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
        if (umenu)
            MUI_DisposeObject(umenu);
        if (gmenu)
            MUI_DisposeObject(gmenu);
        MUI_DeleteCustomClass(listclass);
        FreeVec(groupnames);
        return FALSE;
    }

    /* the context menus: the same actions as the buttons */
    {
        struct { Object *item; ULONG id; } menuids[] =
        {
            { um_edit, ID_EDIT }, { um_pw, ID_PASSWORD }, { um_lock, ID_LOCK }, { um_unlock, ID_LOCK },
            { um_del, ID_DELETE }, { um_add, ID_ADD },
            { gm_edit, ID_GEDIT }, { gm_del, ID_GDELETE }, { gm_add, ID_GADD }
        };
        for (i = 0; i < sizeof(menuids) / sizeof(menuids[0]); i++)
            DoMethod(menuids[i].item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                     (IPTR)app, 2, MUIM_Application_ReturnID, menuids[i].id);
    }

    {
        struct { Object *w, *save, *cancel; ULONG sid, cid; } dlg[] =
        {
            { ewin, e_save, e_cancel, ID_EDIT_SAVE,  ID_EDIT_CANCEL  },
            { gwin, g_save, g_cancel, ID_GEDIT_SAVE, ID_GEDIT_CANCEL },
            { pwin, p_save, p_cancel, ID_PW_SAVE,    ID_PW_CANCEL    },
            { mwin, m_save, m_cancel, ID_MYPW_SAVE,  ID_MYPW_CANCEL  }
        };
        for (i = 0; i < 4; i++)
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
    DoMethod(p_remove, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_PW_REMOVE);
    DoMethod(bt_login, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_LOGIN);
    DoMethod(bt_refresh, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_REFRESH);
    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, ID_SELECT);
    DoMethod(list, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_EDIT);
    DoMethod(bt_gadd, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_GADD);
    DoMethod(bt_gedit, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_GEDIT);
    DoMethod(bt_gdel, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_GDELETE);
    DoMethod(glist, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, ID_GSELECT);
    DoMethod(glist, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (IPTR)app, 2, MUIM_Application_ReturnID, ID_GEDIT);

    /* a group description is kept by security.library only */
    set(g_desclabel, MUIA_ShowMe, st->Multi);
    set(g_desc, MUIA_ShowMe, st->Multi);

    Refresh(NULL, NULL);
    set(win, MUIA_Window_Open, TRUE);

    /* a single-user system without accounts or groups: offer the
     * standard ones of a network environment */
    if (Acc_NeedsDefaults(st) &&
        MUI_RequestA(app, win, 0, (STRPTR)MSG(MSG_TITLE), (STRPTR)MSG(MSG_REQ_DEFAULTS_BUTTONS),
                     (STRPTR)MSG(MSG_REQ_DEFAULTS), NULL) == 1)
    {
        LONG err = Acc_AddDefaults(st);

        Refresh(NULL, NULL);
        if (err)
            ErrorMessage(err, FALSE);
    }

    while (running)
    {
        ULONG id = DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);
        struct Account *a = Selected();
        struct GroupEntry *g = SelectedGroup();

        switch (id)
        {
        case MUIV_Application_ReturnID_Quit:
            running = FALSE;
            break;
        case ID_SELECT:
            UpdateButtons();
            break;
        case ID_ADD:
            if (Acc_CanEdit(st))
                OpenEdit(NULL);
            break;
        case ID_EDIT:
            if (Acc_CanEdit(st) && a)
                OpenEdit(a);
            break;
        case ID_PASSWORD:
            if (Acc_CanEdit(st) && a)
                OpenPassword(a);
            break;
        case ID_LOCK:
            if (Acc_CanEdit(st) && a)
            {
                char keep[ACC_NAMESIZE];
                LONG err;

                strncpy(keep, a->Name, sizeof(keep) - 1);
                keep[sizeof(keep) - 1] = '\0';
                if ((err = Acc_Lock(st, a->Name, !a->Locked)))
                    ErrorMessage(err, FALSE);
                Refresh(keep, NULL);
            }
            break;
        case ID_DELETE:
            if (Acc_CanEdit(st) && a)
            {
                char text[256];
                LONG err;

                snprintf(text, sizeof(text), MSG(MSG_REQ_DELETE), a->Name);
                if (MUI_RequestA(app, win, 0, (STRPTR)MSG(MSG_TITLE), (STRPTR)MSG(MSG_REQ_YESNO), text, NULL) == 1)
                {
                    if ((err = Acc_Delete(st, a->Name)))
                        ErrorMessage(err, FALSE);
                    Refresh(NULL, NULL);
                }
            }
            break;
        case ID_MYPW:
            set(mwin, MUIA_Window_Open, TRUE);
            set(mwin, MUIA_Window_ActiveObject, (IPTR)m_old);
            break;
        case ID_LOGIN:
            if (Acc_LoginRoot(st))
                Refresh(a ? a->Name : NULL, NULL);
            break;
        case ID_REFRESH:
            Refresh(a ? a->Name : NULL, NULL);
            break;
        case ID_GSELECT:
            UpdateButtons();
            break;
        case ID_GADD:
            if (Acc_CanEdit(st))
                OpenGroupEdit(NULL);
            break;
        case ID_GEDIT:
            if (Acc_CanEdit(st) && g)
                OpenGroupEdit(g);
            break;
        case ID_GDELETE:
            if (Acc_CanEdit(st) && g)
            {
                char text[256];
                LONG err;

                snprintf(text, sizeof(text), MSG(MSG_REQ_DELETE_GROUP), g->Name);
                if (MUI_RequestA(app, win, 0, (STRPTR)MSG(MSG_TITLE), (STRPTR)MSG(MSG_REQ_YESNO), text, NULL) == 1)
                {
                    if ((err = Acc_GroupDelete(st, g->Name)))
                        ErrorMessage(err, TRUE);
                    Refresh(a ? a->Name : NULL, NULL);
                }
            }
            break;
        case ID_GEDIT_SAVE:
            SaveGroupEdit();
            break;
        case ID_GEDIT_CANCEL:
            set(gwin, MUIA_Window_Open, FALSE);
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
            ClosePassword();
            break;
        case ID_PW_REMOVE:
            RemovePassword();
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
    /* not disposed with the lists */
    MUI_DisposeObject(umenu);
    MUI_DisposeObject(gmenu);
    MUI_DeleteCustomClass(listclass);
    FreeVec(groupnames);
    groupnames = NULL;
    return TRUE;
}
