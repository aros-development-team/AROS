/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Imports - the window: host, its volumes, connect mode,
          Connect. Logins go through envoy.library's login requester; only
          the user name and the encrypted form of the password are kept.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <proto/envoy.h>
#include <libraries/mui.h>
#include <envoy/envoy.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "imports.h"
#include "locale.h"

struct Library *MUIMasterBase;
extern struct Library *EnvoyBase;

enum { EV_SELECTHOST = 1, EV_LIST, EV_HOSTENTER, EV_VOLUME, EV_CONNECT, EV_QUIT };

static Object *app, *win, *hoststr, *volumes, *mode, *connectbtn, *status;
static struct ImpLogin login;           /* user + "$"-hash of the last listing */
static char listedhost[IMP_HOSTLEN];
static char lastuser[32];
static char statusbuf[300];

static void Status(CONST_STRPTR fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(statusbuf, sizeof(statusbuf), fmt, ap);
    va_end(ap);
    set(status, MUIA_Text_Contents, (IPTR)statusbuf);
}

static struct Window *IWindow(void)
{
    IPTR w = 0;

    get(win, MUIA_Window_Window, &w);
    return (struct Window *)w;
}

static void GetHost(STRPTR buf, ULONG size)
{
    IPTR s = 0;

    get(hoststr, MUIA_String_Contents, &s);
    buf[0] = '\0';
    if (s)
    {
        strncpy(buf, (CONST_STRPTR)s, size - 1);
        buf[size - 1] = '\0';
    }
}

static void SelectHost(void)
{
    char host[IMP_HOSTLEN];

    if (!EnvoyBase)
        return;
    GetHost(host, sizeof(host));
    if (HostRequest(HREQ_Buffer, (IPTR)host, HREQ_BuffSize, sizeof(host),
                    HREQ_Window, (IPTR)IWindow(), HREQ_OptimWindow, (IPTR)IWindow(), TAG_DONE))
        set(hoststr, MUIA_String_Contents, (IPTR)host);
}

static void ListVolumes(void)
{
    char host[IMP_HOSTLEN], user[32], password[32], title[IMP_HOSTLEN + 40], err[64];
    struct List list;
    struct Node *n;
    ULONG rc;
    LONG count = 0;

    GetHost(host, sizeof(host));
    if (!host[0])
    {
        Status(_(MSG_STATUS_NOHOST));
        return;
    }
    if (!EnvoyBase)
    {
        Status(_(MSG_ERR_NOLIB), "envoy.library");
        return;
    }
    user[0] = password[0] = '\0';
    snprintf(title, sizeof(title), _(MSG_LOGINTITLE), host);
    if (!LoginRequest(LREQ_NameBuff, (IPTR)user, LREQ_NameBuffLen, sizeof(user),
                      LREQ_PassBuff, (IPTR)password, LREQ_PassBuffLen, sizeof(password),
                      lastuser[0] ? LREQ_UserName : TAG_IGNORE, (IPTR)lastuser,
                      LREQ_Title, (IPTR)title,
                      LREQ_Window, (IPTR)IWindow(), LREQ_OptimWindow, (IPTR)IWindow(), TAG_DONE))
        return;

    DoMethod(volumes, MUIM_List_Clear);
    set(connectbtn, MUIA_Disabled, TRUE);
    listedhost[0] = '\0';
    Status(_(MSG_STATUS_LISTING), host);
    set(app, MUIA_Application_Sleep, TRUE);

    NEWLIST(&list);
    rc = Imp_ListExports(host, user, password, &list);
    Imp_MakeLogin(&login, user, password);      /* keep only the encrypted form */
    memset(password, 0, sizeof(password));
    strcpy(lastuser, user);

    set(app, MUIA_Application_Sleep, FALSE);
    if (rc)
    {
        Status("%s: %s", host, Imp_ErrorText(rc, err, sizeof(err)));
        return;
    }
    for (n = list.lh_Head; n->ln_Succ; n = n->ln_Succ, count++)
        DoMethod(volumes, MUIM_List_InsertSingle, (IPTR)n->ln_Name, MUIV_List_Insert_Bottom);
    Imp_FreeList(&list);
    strcpy(listedhost, host);
    if (count)
        Status(_(MSG_STATUS_LISTED), (long)count, host);
    else
        Status(_(MSG_STATUS_EMPTY), host);
}

static void Connect(void)
{
    char path[300], vol[108], dev[IMP_HOSTLEN + IMP_NAMELEN + 2], reason[100];
    STRPTR export = NULL;
    IPTR location = 0;
    LONG err;

    DoMethod(volumes, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (IPTR)&export);
    if (!export || !listedhost[0])
    {
        Status(_(MSG_STATUS_NOVOLUME));
        return;
    }
    get(mode, MUIA_Cycle_Active, &location);
    Imp_DeviceName(dev, sizeof(dev), listedhost, export);
    Status(_(MSG_STATUS_MOUNTING), export);
    set(app, MUIA_Application_Sleep, TRUE);
    err = Imp_Mount(listedhost, export, &login, (LONG)location, path, sizeof(path), vol, sizeof(vol));
    set(app, MUIA_Application_Sleep, FALSE);
    if (err == ERROR_OBJECT_EXISTS)
        Status(_(MSG_STATUS_ALREADY), dev);
    else if (err == ERROR_WRITE_PROTECTED || err == ERROR_DISK_FULL || err == ERROR_DIR_NOT_FOUND)
        Status(_(MSG_STATUS_WRITEFAIL), path);
    else if (err)
    {
        if (err == IMP_ERR_REFUSED)
        {
            char tmp[64];
            strncpy(reason, Imp_ErrorText(err, tmp, sizeof(tmp)), sizeof(reason) - 1);
            reason[sizeof(reason) - 1] = '\0';
        }
        else
            Fault(err, NULL, reason, sizeof(reason));
        Status(_(MSG_STATUS_MOUNTFAIL), dev, reason);
    }
    else
        Status(_(MSG_STATUS_MOUNTED), vol, dev);
}

int Gui_Run(CONST_STRPTR host, CONST_STRPTR user, CONST_STRPTR pubscreen)
{
    static CONST_STRPTR modes[4];
    struct Screen *screen = NULL;
    Object *selectbtn, *listbtn, *quitbtn;
    ULONG sigs = 0;
    BOOL running = TRUE;

    if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0)))
    {
        Printf(_(MSG_ERR_NOLIB), "muimaster.library");
        PutStr("\n");
        return RETURN_FAIL;
    }
    modes[0] = _(MSG_MODE_TEMPORARY);
    modes[1] = _(MSG_MODE_PERMANENT);
    modes[2] = _(MSG_MODE_STORAGE);
    modes[3] = NULL;
    if (user)
    {
        strncpy(lastuser, user, sizeof(lastuser) - 1);
        lastuser[sizeof(lastuser) - 1] = '\0';
    }
    if (pubscreen)
        screen = LockPubScreen(pubscreen);

    app = ApplicationObject,
        MUIA_Application_Title, __(MSG_WINTITLE),
        MUIA_Application_Version, (IPTR)"$VER: FilesystemImports 50.0 (8.10.2026)",
        MUIA_Application_Base, (IPTR)"EFSIMPORTS",
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, __(MSG_WINTITLE),
            MUIA_Window_ID, MAKE_ID('E', 'F', 'S', 'I'),
            screen ? MUIA_Window_Screen : TAG_IGNORE, (IPTR)screen,
            WindowContents, (IPTR)(VGroup,
                Child, (IPTR)(HGroup,
                    Child, (IPTR)Label2(_(MSG_HOST)),
                    Child, (IPTR)(hoststr = StringObject, StringFrame,
                        MUIA_String_MaxLen, IMP_HOSTLEN,
                        MUIA_String_Contents, (IPTR)(host ? host : (CONST_STRPTR)""),
                        MUIA_CycleChain, 1,
                    End),
                    Child, (IPTR)(selectbtn = SimpleButton(_(MSG_SELECTHOST))),
                    Child, (IPTR)(listbtn = SimpleButton(_(MSG_LIST))),
                End),
                Child, (IPTR)(volumes = ListviewObject,
                    MUIA_Frame, MUIV_Frame_InputList,
                    MUIA_FrameTitle, __(MSG_VOLUMES),
                    MUIA_CycleChain, 1,
                    MUIA_Listview_List, (IPTR)(ListObject,
                        MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                        MUIA_List_DestructHook, MUIV_List_DestructHook_String,
                    End),
                End),
                Child, (IPTR)(HGroup,
                    Child, (IPTR)Label1(_(MSG_MODE)),
                    Child, (IPTR)(mode = CycleObject, MUIA_Cycle_Entries, (IPTR)modes, MUIA_CycleChain, 1, End),
                End),
                Child, (IPTR)(status = TextObject, TextFrame, MUIA_Background, MUII_TextBack,
                    MUIA_Text_Contents, __(MSG_STATUS_READY),
                End),
                Child, (IPTR)(HGroup, MUIA_Group_SameWidth, TRUE,
                    Child, (IPTR)(connectbtn = SimpleButton(_(MSG_CONNECT))),
                    Child, (IPTR)HSpace(0),
                    Child, (IPTR)(quitbtn = SimpleButton(_(MSG_QUIT))),
                End),
            End),
        End),
    End;

    if (!app)
    {
        if (screen)
            UnlockPubScreen(NULL, screen);
        CloseLibrary(MUIMasterBase);
        return RETURN_FAIL;
    }
    set(connectbtn, MUIA_Disabled, TRUE);
    if (!EnvoyBase)
        set(selectbtn, MUIA_Disabled, TRUE);

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)app, 2, MUIM_Application_ReturnID, EV_QUIT);
    DoMethod(quitbtn, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, EV_QUIT);
    DoMethod(selectbtn, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, EV_SELECTHOST);
    DoMethod(listbtn, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, EV_LIST);
    DoMethod(hoststr, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, EV_HOSTENTER);
    DoMethod(connectbtn, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 2, MUIM_Application_ReturnID, EV_CONNECT);
    DoMethod(volumes, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_ReturnID, EV_VOLUME);
    DoMethod(volumes, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, (IPTR)app, 2, MUIM_Application_ReturnID, EV_CONNECT);

    set(win, MUIA_Window_Open, TRUE);
    set(win, MUIA_Window_ActiveObject, (IPTR)hoststr);
    while (running)
    {
        ULONG id = DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs);
        switch (id)
        {
        case MUIV_Application_ReturnID_Quit:
        case EV_QUIT:
            running = FALSE;
            break;
        case EV_SELECTHOST:
            SelectHost();
            break;
        case EV_HOSTENTER:
        case EV_LIST:
            ListVolumes();
            break;
        case EV_VOLUME:
        {
            IPTR active = MUIV_List_Active_Off;
            get(volumes, MUIA_List_Active, &active);
            set(connectbtn, MUIA_Disabled, (LONG)active < 0);
            break;
        }
        case EV_CONNECT:
            Connect();
            break;
        }
        if (running && sigs)
        {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C);
            if (sigs & SIGBREAKF_CTRL_C)
                running = FALSE;
        }
    }
    memset(&login, 0, sizeof(login));
    MUI_DisposeObject(app);
    if (screen)
        UnlockPubScreen(NULL, screen);
    CloseLibrary(MUIMasterBase);
    return RETURN_OK;
}
