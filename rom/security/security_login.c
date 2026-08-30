/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.

    Desc: security.library login/logout. Derived from MultiUser Log.c
          (c) Geert Uytterhoeven.

          A graphical login uses an intuition requester when possible, the
          console login talks to the secT_Input/secT_Output filehandles.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <proto/security.h>

#include <dos/var.h>
#include <exec/memory.h>
#include <dos/datetime.h>
#include <intuition/intuition.h>

#include "security_intern.h"
#include "security_task.h"
#include "security_login.h"
#include "security_server.h"
#include "security_userinfo.h"
#include "security_memory.h"
#include "security_support.h"

/*
 * Tag parsing
 */
BOOL InterpretTagList(struct SecurityBase *secBase, struct TagItem *taglist, struct secTags *tags)
{
    struct TagItem *tstate = taglist;
    struct TagItem *tag;
    struct Task *me = FindTask(NULL);
    BOOL isproc = (me->tc_Node.ln_Type == NT_PROCESS);

    tags->Input = isproc ? Input() : BNULL;
    tags->Output = isproc ? Output() : BNULL;
    tags->PubScrName = NULL;
    tags->Task = me;
    tags->UserID = NULL;
    tags->Password = NULL;
    tags->DefProtection = secDEFPROTECTION;
    tags->Graphical = FALSE;
    tags->Own = FALSE;
    tags->Global = FALSE;
    tags->Quiet = FALSE;
    tags->All = FALSE;
    tags->NoLog = FALSE;

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case secT_Input:         tags->Input = (BPTR)tag->ti_Data;              break;
        case secT_Output:        tags->Output = (BPTR)tag->ti_Data;             break;
        case secT_Graphical:     tags->Graphical = tag->ti_Data ? TRUE : FALSE; break;
        case secT_PubScrName:    tags->PubScrName = (STRPTR)tag->ti_Data;       break;
        case secT_Task:          if (tag->ti_Data) tags->Task = (struct Task *)tag->ti_Data; break;
        case secT_Own:           tags->Own = tag->ti_Data ? TRUE : FALSE;       break;
        case secT_Global:        tags->Global = tag->ti_Data ? TRUE : FALSE;    break;
        case secT_Quiet:         tags->Quiet = tag->ti_Data ? TRUE : FALSE;     break;
        case secT_UserID:        tags->UserID = (STRPTR)tag->ti_Data;           break;
        case secT_Password:      tags->Password = (STRPTR)tag->ti_Data;         break;
        case secT_DefProtection: tags->DefProtection = tag->ti_Data;            break;
        case secT_All:           tags->All = tag->ti_Data ? TRUE : FALSE;       break;
        case secT_NoLog:         tags->NoLog = tag->ti_Data ? TRUE : FALSE;     break;
        case secT_System:        tags->System = tag->ti_Data ? TRUE : FALSE;    break;
        }
    }

    /* Only root may switch off logging or touch other people's tasks */
    if (tags->NoLog && !CallerIsRoot(secBase))
        return FALSE;
    if (tags->Task != me)
    {
        ULONG owner = GetTaskOwner(secBase, tags->Task);
        ULONG mine = GetTaskOwner(secBase, me);

        if (!CallerIsRoot(secBase) && owner != secOWNER_NOBODY && owner != mine)
            return FALSE;
    }
    /* A graphical login needs a screen */
    if (tags->Graphical && !(secBase->sec_AfterDOSDone && IntuitionBase && IntuitionBase->FirstScreen))
        tags->Graphical = FALSE;
    return TRUE;
}

/*
 * Console I/O
 */
static void myfputs(struct SecurityBase *secBase, BPTR file, CONST_STRPTR str)
{
    if (file && str)
    {
        FPuts(file, str);
        Flush(file);
    }
}

BOOL ReadLineCon(struct SecurityBase *secBase, BPTR input, STRPTR buf, ULONG size)
{
    ULONG len;

    if (!input || !FGets(input, buf, size))
    {
        buf[0] = '\0';
        return FALSE;
    }
    len = strlen(buf);
    while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        buf[--len] = '\0';
    return len > 0;
}

/* Read a password without echo. Returns FALSE on EOF/break. */
BOOL ReadPasswordCon(struct SecurityBase *secBase, BPTR input, BPTR output, STRPTR buf, ULONG size, struct LocaleInfo *li)
{
    ULONG len = 0;
    BOOL done = FALSE, ok = TRUE;
    LONG c;

    if (!input)
        return FALSE;

    if (li)
        myfputs(secBase, output, GetLocS(secBase, li, MSG_PASSWDPROMPT_CON));
    myfputs(secBase, output, " ");

    SetMode(input, 1);
    do
    {
        c = FGetC(input);
        switch (c)
        {
        case -1:
        case 3:                         /* CTRL-C */
            ok = FALSE;
            done = TRUE;
            break;
        case '\b':
        case 127:
            if (len)
                len--;
            break;
        case '\n':
        case '\r':
            done = TRUE;
            break;
        default:
            if ((len < size - 1) && (c & 0x7f) > 31)
                buf[len++] = c;
            else
                FPutC(output, 7);
            Flush(output);
            break;
        }
    } while (!done);
    buf[len] = '\0';
    SetMode(input, 0);
    myfputs(secBase, output, "\n");
    return ok;
}

/* Does the user have a password at all? (unknown users: yes) */
static BOOL UserHasPassword(struct SecurityBase *secBase, CONST_STRPTR userid)
{
    struct secPrivUserInfo *info;
    BOOL pwd = TRUE;

    if ((info = (struct secPrivUserInfo *)secAllocUserInfo()))
    {
        strncpy(info->Pub.UserID, userid, secUSERIDSIZE - 1);
        info->Pub.UserID[secUSERIDSIZE - 1] = '\0';
        if (SendServerPacket(secBase, secSAction_GetUserInfo, (SIPTR)info, secKeyType_UserID, 0, 0))
            pwd = info->Password;
        secFreeUserInfo((struct secUserInfo *)info);
    }
    return pwd;
}

/*
 * Graphical prompts: a small intuition window with one string gadget
 */

/*
 * Ask the user for a UserID / password until a valid pair is entered.
 * failallowed: give up after one failed attempt (secLoginA).
 * nopasswd:    root may login as anybody without a password.
 */
static struct secPrivUserInfo *LoginRequest(struct SecurityBase *secBase, struct secTags *tags, BOOL failallowed, BOOL nopasswd, struct LocaleInfo *li)
{
    char version[16];
    char hostname[32];
    char uidbuf[secUSERIDSIZE];
    char pwdbuf[secPASSWORDSIZE];
    char text[256];
    STRPTR userid, password;
    struct secPrivUserInfo *info = NULL;
    int retry = 0, i;
    BOOL ret;
    BOOL guipwd = FALSE;    /* the password came with the user id from the login window */

    if (GetVar("Kickstart", version, sizeof(version), GVF_GLOBAL_ONLY) == -1)
        strcpy(version, "?");
    if (GetVar("HostName", hostname, sizeof(hostname), GVF_GLOBAL_ONLY) == -1)
        strcpy(hostname, "?");
    else
    {
        for (i = 0; hostname[i] && (hostname[i] != '.'); i++);
        hostname[i] = '\0';
    }

    do
    {
        if (tags->UserID)
            userid = tags->UserID;
        else
        {
            userid = uidbuf;
            do
            {
                SIPTR args[2] = { (SIPTR)version, (SIPTR)hostname };

                memset(uidbuf, 0, sizeof(uidbuf));
                if (tags->Graphical)
                {
                    /* the login window asks for user id and password at once */
                    FormatString(GetLocS(secBase, li, MSG_LOGINPROMPT_GUI), args, text, sizeof(text));
                    memset(pwdbuf, 0, sizeof(pwdbuf));
                    LONG g = LoginGUI(secBase, tags->PubScrName, text, failallowed, tags->System,
                                      uidbuf, sizeof(uidbuf), pwdbuf, sizeof(pwdbuf));
                    if (g == LOGINGUI_UNAVAILABLE)
                    {
                        /* no MUI (yet): ask on the console instead */
                        tags->Graphical = FALSE;
                        ret = FALSE;
                        continue;
                    }
                    ret = (g == LOGINGUI_OK);
                    guipwd = ret;
                }
                else
                {
                    if (!retry)
                    {
                        FormatString(GetLocS(secBase, li, MSG_LOGINREQ_CON), args, text, sizeof(text));
                        myfputs(secBase, tags->Output, text);
                    }
                    myfputs(secBase, tags->Output, GetLocS(secBase, li, MSG_LOGINPROMPT_CON));
                    myfputs(secBase, tags->Output, " ");
                    ret = ReadLineCon(secBase, tags->Input, uidbuf, sizeof(uidbuf));
                    retry = ret ? (retry + 1) % 4 : 0;
                    if (!ret && !tags->Input)
                        return NULL;        /* nothing to read from */
                }
                if (!ret && failallowed)
                    return NULL;
            } while (!ret);
        }

        if (tags->Password)
            password = tags->Password;
        else
        {
            password = pwdbuf;
            if (!guipwd)
                memset(pwdbuf, 0, sizeof(pwdbuf));
            if (!nopasswd && UserHasPassword(secBase, userid))
            {
                if (guipwd)
                    ret = TRUE;
                else if (tags->Graphical)
                {
                    /* user id given by the caller: the window only needs the password */
                    SIPTR args[2] = { (SIPTR)version, (SIPTR)hostname };
                    FormatString(GetLocS(secBase, li, MSG_LOGINPROMPT_GUI), args, text, sizeof(text));
                    CopyMem(userid, uidbuf, strlen(userid) + 1 > sizeof(uidbuf) ? sizeof(uidbuf) : strlen(userid) + 1);
                    uidbuf[sizeof(uidbuf) - 1] = '\0';
                    LONG g = LoginGUI(secBase, tags->PubScrName, text, failallowed, tags->System,
                                      uidbuf, sizeof(uidbuf), pwdbuf, sizeof(pwdbuf));
                    if (g == LOGINGUI_UNAVAILABLE)
                    {
                        tags->Graphical = FALSE;
                        ret = ReadPasswordCon(secBase, tags->Input, tags->Output, pwdbuf, sizeof(pwdbuf), li);
                    }
                    else
                        ret = (g == LOGINGUI_OK);
                }
                else
                    ret = ReadPasswordCon(secBase, tags->Input, tags->Output, pwdbuf, sizeof(pwdbuf), li);
                if (!ret)
                {
                    if (failallowed)
                        return NULL;
                    password = NULL;
                }
            }
        }

        if (userid && password)
        {
            info = (struct secPrivUserInfo *)SendServerPacket(secBase, secSAction_CheckUser, (SIPTR)userid,
                                                              (SIPTR)password, nopasswd, tags->NoLog);
            memset(pwdbuf, 0, sizeof(pwdbuf));
            guipwd = FALSE;
            if (!info)
            {
                if (failallowed)
                    return NULL;
                if (tags->Graphical)
                    LoginMessageGUI(secBase, GetLocS(secBase, li, MSG_LOGINREQ_GUI),
                                    GetLocS(secBase, li, MSG_LOGINFAIL_GUI), GetLocS(secBase, li, MSG_OK));
                else
                    myfputs(secBase, tags->Output, GetLocS(secBase, li, MSG_LOGINFAIL_CON));
            }
        }
    } while (!info);

    return info;
}

/*
 * Apply an owner to a task, or to a task and all its descendants
 * (secT_Global), pushing the previous credentials. TaskOwnerSem held.
 */
static void ApplyOwner(struct SecurityBase *secBase, struct secTaskNode *node, const struct secExtOwner *owner, struct secSession *session, BOOL push, BOOL recurse)
{
    struct MinNode *n;

    if (push)
        PushOwner(secBase, node);
    SetNodeOwner(secBase, node, owner);
    if (session)
        JoinSession(secBase, node, session);
    if (recurse)
    {
        ForeachNode(&node->Children, n)
            ApplyOwner(secBase, TASKNODE_FROM_SIBLINGS(n), owner, session, push, TRUE);
    }
}

static BOOL RestoreOwner(struct SecurityBase *secBase, struct secTaskNode *node, BOOL recurse)
{
    struct MinNode *n;
    BOOL res;

    res = PopOwner(secBase, node);
    if (recurse)
    {
        ForeachNode(&node->Children, n)
            PopOwner(secBase, TASKNODE_FROM_SIBLINGS(n));
    }
    return res;
}

/*
 * Local shell variables live in a process' pr_LocalVars and SetVar() only
 * reaches the calling process. Set one in another process - the shell that
 * runs Security-Startup when "Login PARENT" is used - the way dos.library
 * does, so that the shell can free it again.
 */
static void SetProcessVar(struct SecurityBase *secBase, struct Task *task, CONST_STRPTR name, CONST_STRPTR value)
{
    struct Process *pr = (struct Process *)task;
    struct LocalVar *lv, *n;
    ULONG nlen = strlen(name), vlen = strlen(value);

    if (!task || task->tc_Node.ln_Type != NT_PROCESS)
        return;
    if (task == FindTask(NULL))
    {
        SetVar(name, value, -1, GVF_LOCAL_ONLY);
        return;
    }
    if (!(lv = AllocVec(sizeof(struct LocalVar) + nlen + 1, MEMF_PUBLIC | MEMF_CLEAR)))
        return;
    lv->lv_Node.ln_Type = LV_VAR;
    lv->lv_Node.ln_Name = (STRPTR)(lv + 1);
    CopyMem(name, lv->lv_Node.ln_Name, nlen + 1);
    lv->lv_Len = vlen;
    if (vlen)
    {
        if (!(lv->lv_Value = AllocMem(vlen, MEMF_PUBLIC)))
        {
            FreeVec(lv);
            return;
        }
        CopyMem(value, lv->lv_Value, vlen);
    }
    Forbid();
    ForeachNode(&pr->pr_LocalVars, n)
    {
        if (n->lv_Node.ln_Type == LV_VAR && !Stricmp(name, n->lv_Node.ln_Name))
        {
            Remove(&n->lv_Node);
            if (n->lv_Len)
                FreeMem(n->lv_Value, n->lv_Len);
            FreeVec(n);
            break;
        }
    }
    AddHead((struct List *)&pr->pr_LocalVars, &lv->lv_Node);
    Permit();
}

/* "$Home"/"$User" = home directory and user id of the user that just logged in */
static void SetHomeVar(struct SecurityBase *secBase, struct Task *task, UWORD uid)
{
    struct secUserInfo *ui = secAllocUserInfo();

    if (ui)
    {
        ui->uid = uid;
        if (secGetUserInfo(ui, secKeyType_uid))
        {
            SetProcessVar(secBase, task, "Home", ui->HomeDir);
            SetProcessVar(secBase, task, "User", ui->UserID);
        }
        else
        {
            SetProcessVar(secBase, task, "Home", "");
            SetProcessVar(secBase, task, "User", "");
        }
        secFreeUserInfo(ui);
    }
}

/* After a fresh login from the logout prompt: home dir, lastlogin, profile */
static void PostLogin(struct SecurityBase *secBase, struct secTags *tags, struct secPrivUserInfo *info, struct LocaleInfo *li)
{
    char day[LEN_DATSTRING], date[LEN_DATSTRING], time[LEN_DATSTRING];
#define LASTLOGINSIZE (3 * LEN_DATSTRING + 2)
    char lastlogin[LASTLOGINSIZE + 1];
    char text[256];
    BPTR file, dir, olddir = BNULL;
    struct DateTime dt;
    SIPTR args[6];
    BOOL neverloggedin = TRUE;
    int i;

    if (FindTask(NULL)->tc_Node.ln_Type != NT_PROCESS)
        return;

    SetVar("Home", info->Pub.HomeDir, -1, GVF_LOCAL_ONLY);
    if (info->Pub.HomeDir[0] && (dir = Lock(info->Pub.HomeDir, ACCESS_READ)))
    {
        if (NameFromLock(dir, text, sizeof(text)))
            SetCurrentDirName(text);
        olddir = CurrentDir(dir);
        if (olddir)
            UnLock(olddir);
    }

    if ((file = Open(secLastLogin_FileName, MODE_OLDFILE)))
    {
        if (FGets(file, lastlogin, LASTLOGINSIZE))
            neverloggedin = FALSE;
        Close(file);
    }
    dt.dat_Format = FORMAT_DOS;
    dt.dat_Flags = 0;
    DateStamp(&dt.dat_Stamp);
    dt.dat_StrDay = day;
    dt.dat_StrDate = date;
    dt.dat_StrTime = time;
    DateToStr(&dt);
    args[0] = (SIPTR)day;
    args[1] = (SIPTR)date;
    args[2] = (SIPTR)time;
    args[3] = args[4] = args[5] = (SIPTR)"";
    if ((file = Open(secLastLogin_FileName, MODE_NEWFILE)))
    {
        VFPrintf(file, "%s %s %s\n", (RAWARG)args);
        Close(file);
    }
    if (!neverloggedin)
    {
        args[3] = (SIPTR)lastlogin;
        for (i = 0; lastlogin[i] && (lastlogin[i] != ' '); i++);
        if (lastlogin[i])
        {
            lastlogin[i++] = '\0';
            while (lastlogin[i] == ' ') i++;
            args[4] = (SIPTR)&lastlogin[i];
            while (lastlogin[i] && (lastlogin[i] != ' ')) i++;
            if (lastlogin[i])
            {
                lastlogin[i++] = '\0';
                while (lastlogin[i] == ' ') i++;
                args[5] = (SIPTR)&lastlogin[i];
                while (lastlogin[i] && (lastlogin[i] != ' ') && (lastlogin[i] != '\n')) i++;
                lastlogin[i] = '\0';
            }
        }
    }

    if (secBase->Config.Flags & secCFGF_LastLoginReq)
    {
        FormatString(GetLocS(secBase, li, neverloggedin ? MSG_FIRSTLOGIN : MSG_LASTLOGIN), args, text, sizeof(text));
        if (tags->Graphical)
            LoginMessageGUI(secBase, GetLocS(secBase, li, MSG_LOGINREQ_GUI), text, GetLocS(secBase, li, MSG_OK));
        else if (tags->Output)
        {
            FPuts(tags->Output, "\n");
            FPuts(tags->Output, text);
            FPuts(tags->Output, "\n");
            Flush(tags->Output);
        }
    }

    /* Run the profile from the configuration directory, if any */
    if ((secBase->Config.Flags & secCFGF_Profile) &&
        (dir = (BPTR)SendServerPacket(secBase, secSAction_ConfigDirLock, 0, 0, 0, 0)))
    {
        olddir = CurrentDir(dir);
        if ((file = Lock(secProfile_FileName, ACCESS_READ)))
        {
            UnLock(file);
            strcpy(text, "Execute " secProfile_FileName);
            Execute(text, tags->Input, tags->Output);
        }
        CurrentDir(olddir);
        UnLock(dir);
    }
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, secLogoutA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 5, Security)

/*  FUNCTION
        Logout and restore the previous user. If there was no previous user
        or the previous user was nobody, a login request will appear (unless
        secT_Quiet). A login from this request also sets the local variable
        "Home", the current directory, shows the last login information and
        executes the .profile from the configuration directory.

    INPUTS
        taglist - see secLoginA(); additionally
            secT_Quiet - (BOOL) never request a login, simply logout.
            secT_All   - (BOOL) logout all previous users.

    RESULT
        The user you are now (uid<<16 | gid), secOWNER_NOBODY for nobody.

    SEE ALSO
        secLoginA()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTags tags;
    struct secTaskNode *node;
    struct secPrivUserInfo *info;
    struct secExtOwner *xuser;
    struct LocaleInfo li;
    BOOL nobody;
    ULONG user;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    if (!InterpretTagList(secBase, taglist, &tags))
        return secOWNER_NOBODY;
    tags.UserID = tags.Password = NULL;
    tags.NoLog = FALSE;

    ObtainSemaphore(&secBase->TaskOwnerSem);
    node = FindOrCreateTaskNode(secBase, tags.Task);
    do
    {
        nobody = node ? !RestoreOwner(secBase, node, tags.Global) : TRUE;
        if (node && !nobody && !node->Owner)
            nobody = TRUE;
    } while (tags.All && !nobody && node && !IsMinListEmpty(&node->OwnerStack));
    if (node && nobody)
    {
        ClearOwnerStack(secBase, node);
        SetNodeOwner(secBase, node, NULL);
        LeaveSession(secBase, node);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    user = GetTaskOwner(secBase, tags.Task);

    if (nobody && !tags.Quiet && secBase->Configured)
    {
        OpenLoc(secBase, &li);
        if ((info = LoginRequest(secBase, &tags, FALSE, FALSE, &li)))
        {
            if ((xuser = secUserInfo2ExtOwner((struct secUserInfo *)info)))
            {
                struct secSession *session;

                ObtainSemaphore(&secBase->TaskOwnerSem);
                if ((node = FindOrCreateTaskNode(secBase, tags.Task)))
                {
                    session = AllocSession(secBase, info->Pub.UserID);
                    if (session)
                        session->Leader = node;
                    ApplyOwner(secBase, node, xuser, session, FALSE, tags.Global);
                }
                ReleaseSemaphore(&secBase->TaskOwnerSem);
                user = secExtOwner2ULONG(xuser);
                secFreeExtOwner(xuser);
                PostLogin(secBase, &tags, info, &li);
            }
            secFreeUserInfo((struct secUserInfo *)info);
        }
        CloseLoc(secBase, &li);
        CloseSystemScreen(secBase);
    }

    return user;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, secLoginA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct SecurityBase *, secBase, 6, Security)

/*  FUNCTION
        Login to the system and remember the previous user of the task.

    TAGS
        secT_Graphical  - (BOOL) use a graphical login instead of a console
                          one. Default FALSE.
        secT_Input      - (BPTR) filehandle to read from. Default Input().
        secT_Output     - (BPTR) filehandle to write to. Default Output().
        secT_PubScrName - (STRPTR) public screen for the requester.
        secT_Task       - (struct Task *) the task to login. Only your own
                          tasks or tasks owned by nobody, unless you are root.
                          Default: the current task.
        secT_Own        - (BOOL) make the task owned by the caller's owner.
        secT_Global     - (BOOL) also login all descendants of the task.
        secT_UserID     - (STRPTR) do not ask for a UserID.
        secT_Password   - (STRPTR) do not ask for a password (needs secT_UserID).
        secT_NoLog      - (BOOL) do not log this action (root only).

    RESULT
        The user that logged in (uid<<16 | gid), or secOWNER_NOBODY for a
        failure.

    NOTES
        On an unconfigured system (no password file) any login succeeds as
        root, so that Security-Startup scripts work before the system is set
        up.

    SEE ALSO
        secLogoutA()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct secTags tags;
    struct secTaskNode *node;
    struct secExtOwner *xuser = NULL;
    struct secSession *session = NULL;
    struct secPrivUserInfo *info = NULL;
    struct LocaleInfo li;
    BOOL isroot;

    D(bug(DEBUG_NAME_STR " %s()\n", __func__);)

    if (!InterpretTagList(secBase, taglist, &tags) || (tags.Password && !tags.UserID))
        return secOWNER_NOBODY;

    isroot = CallerIsRoot(secBase);

    if (tags.Own)
    {
        if (!(xuser = GetTaskExtOwner(secBase, FindTask(NULL))))
            return secOWNER_NOBODY;
    }
    else if (!secBase->Configured)
    {
        /* Unconfigured: everybody is root */
        xuser = CloneExtOwner(&RootExtOwner);
    }
    else
    {
        OpenLoc(secBase, &li);
        if ((info = LoginRequest(secBase, &tags, TRUE, isroot, &li)))
            xuser = secUserInfo2ExtOwner((struct secUserInfo *)info);
        CloseLoc(secBase, &li);
        if (!xuser)
        {
            /* cancelled: nothing keeps the login screen up any more */
            CloseSystemScreen(secBase);
            if (info)
                secFreeUserInfo((struct secUserInfo *)info);
            return secOWNER_NOBODY;
        }
        /* on success the "SYSTEM" screen stays up for the rest of the
         * boot script; dos/boot.c closes it before the Startup-Sequence */
    }

    ObtainSemaphore(&secBase->TaskOwnerSem);
    if ((node = FindOrCreateTaskNode(secBase, tags.Task)))
    {
        if (info && (session = AllocSession(secBase, info->Pub.UserID)))
            session->Leader = node;
        ApplyOwner(secBase, node, xuser, session, TRUE, tags.Global);
    }
    ReleaseSemaphore(&secBase->TaskOwnerSem);

    /* $Home for the shell that logged in (Security-Startup: Login PARENT) */
    if (node)
        SetHomeVar(secBase, tags.Task, xuser->uid);

    secFreeExtOwner(xuser);
    if (info)
        secFreeUserInfo((struct secUserInfo *)info);

    D(bug(DEBUG_NAME_STR " %s: task %p ('%s') now owned by %08lx\n", __func__, tags.Task, tags.Task->tc_Node.ln_Name, (unsigned long)GetTaskOwner(secBase, tags.Task));)
    return GetTaskOwner(secBase, tags.Task);

    AROS_LIBFUNC_EXIT
}
