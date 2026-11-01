/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Notify() - hand a pop-up to the notification server.
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "application_intern.h"
#include "application_server.h"
#include LC_LIBDEFS_FILE

static void ntf_Copy(STRPTR dst, CONST_STRPTR src, ULONG size)
{
    if (!src)
        src = "";
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

/* Start the server if it is installed but not running, and wait up to
   ~3 s for its port. SystemTagList() with SYS_Asynch does not report a
   missing program, so its presence is checked first. */
static BOOL ntf_StartServer(void)
{
    struct TagItem tags[] =
    {
        { SYS_Asynch,   TRUE },
        { SYS_Input,    0    },
        { SYS_Output,   0    },
        { NP_Name,      (IPTR)"Notifications" },
        { TAG_DONE,     0    }
    };
    BPTR in, out, lock;
    LONG tries;

    if (FindPort(NOTIFYSERVER_PORTNAME))
        return TRUE;
    if (!(lock = Lock(NOTIFYSERVER_PATH, SHARED_LOCK)))
        return FALSE;
    UnLock(lock);

    in = Open("NIL:", MODE_OLDFILE);
    out = Open("NIL:", MODE_NEWFILE);
    if (!in || !out)
    {
        if (in)
            Close(in);
        if (out)
            Close(out);
        return FALSE;
    }
    tags[1].ti_Data = (IPTR)in;
    tags[2].ti_Data = (IPTR)out;

    /* With SYS_Asynch the new process owns the handles from here on. */
    if (SystemTagList(NOTIFYSERVER_PATH, tags) == -1)
    {
        Close(in);
        Close(out);
        return FALSE;
    }

    for (tries = 0; tries < 60; tries++)
    {
        if (FindPort(NOTIFYSERVER_PORTNAME))
            return TRUE;
        Delay(3);
    }
    return FALSE;
}

/*****************************************************************************

    NAME */
#include <proto/application.h>

        AROS_LH2(ULONG, NotifyA,

/*  SYNOPSIS */
        AROS_LHA(ULONG, appID, D0),
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct ApplicationBase *, AppBase, 11, Application)

/*  FUNCTION
        Show a pop-up notification for a registered application, and add it
        to the notification log. It closes by itself after a few seconds
        (a priority 10 one stays until closed) and asks nothing of the user.

        Double-clicking it sends APPNOTIFY_BackMsg to the application as an
        APPLIBMT_CustomMsg; a BackMsg starting with "URL:" opens the URL in
        the default browser instead.

        The notification server, SYS:System/Notifications, is started if
        it is not running. How pop-ups look and which applications may show
        them is set in the Notifications preferences.

    INPUTS
        appID - the application ID from RegisterApplication().
        tags  - APPNOTIFY_ tags; at least a title or a text, unless
                APPNOTIFY_Withdraw takes back earlier pop-ups.

    RESULT
        APPNOTIFY_OK_MSGQUEUED (0), or an APPNOTIFY_ERROR_ code:
        _APPNOTREGISTERED, _TAGS, _GAMEMODEACTIVE (nothing is shown while
        an application is in game mode), _APPNOTALLOWED (switched off in
        the preferences), _TOOMANYMESSAGESQUEUED, _SERVERNOTRUNNING or
        _OUTOFMEM.

    NOTES
        Use notifications sparingly, keep them short, and never use one for
        an error or a question: the user may never see it.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct NotifyServerMsg *nsm;
    struct MsgPort *reply, *server;
    struct AppEntry *ae;
    CONST_STRPTR text = (CONST_STRPTR)GetTagData(APPNOTIFY_Text, 0, tags);
    CONST_STRPTR title = (CONST_STRPTR)GetTagData(APPNOTIFY_Title, 0, tags);
    BOOL logonly = GetTagData(APPNOTIFY_LogOnly, FALSE, tags) ? TRUE : FALSE;
    ULONG pri = GetTagData(APPNOTIFY_Pri, 0, tags);
    BOOL gamemode;
    ULONG result = APPNOTIFY_ERROR_SERVERNOTRUNNING;

    CONST_STRPTR withdraw = (CONST_STRPTR)GetTagData(APPNOTIFY_Withdraw, 0, tags);

    if (!text && !title && !withdraw)
        return APPNOTIFY_ERROR_TAGS;

    if (!(nsm = AllocVec(sizeof(*nsm), MEMF_PUBLIC | MEMF_CLEAR)))
        return APPNOTIFY_ERROR_OUTOFMEM;

    ObtainSemaphore(&AppBase->ab_Lock);
    app_Reap(AppBase);
    if ((ae = app_Find(AppBase, appID)))
        ntf_Copy(nsm->nsm_AppName, ae->ae_Name, sizeof(nsm->nsm_AppName));
    gamemode = app_GameModeActive(AppBase);
    ReleaseSemaphore(&AppBase->ab_Lock);

    if (!ae)
    {
        FreeVec(nsm);
        return APPNOTIFY_ERROR_APPNOTREGISTERED;
    }
    if (gamemode && !logonly && !withdraw)
    {
        FreeVec(nsm);
        return APPNOTIFY_ERROR_GAMEMODEACTIVE;
    }

    nsm->nsm_Version = NOTIFYSERVER_VERSION;
    nsm->nsm_AppID = appID;
    nsm->nsm_CloseOnDC = GetTagData(APPNOTIFY_CloseOnDC, FALSE, tags) ? TRUE : FALSE;
    nsm->nsm_Update = GetTagData(APPNOTIFY_Update, FALSE, tags) ? TRUE : FALSE;
    nsm->nsm_ConfirmClose = GetTagData(APPNOTIFY_ConfirmClose, FALSE, tags) ? TRUE : FALSE;
    nsm->nsm_LogOnly = logonly;
    nsm->nsm_Pri = pri > 10 ? 10 : pri;
    nsm->nsm_ImageVertAlign = GetTagData(APPNOTIFY_ImageVertAlign, 0, tags);
    ntf_Copy(nsm->nsm_Title, title ? title : (CONST_STRPTR)nsm->nsm_AppName, sizeof(nsm->nsm_Title));
    ntf_Copy(nsm->nsm_Text, text, sizeof(nsm->nsm_Text));
    ntf_Copy(nsm->nsm_PubScreen, (CONST_STRPTR)GetTagData(APPNOTIFY_PubScreenName, 0, tags),
             sizeof(nsm->nsm_PubScreen));
    ntf_Copy(nsm->nsm_ImageFile, (CONST_STRPTR)GetTagData(APPNOTIFY_ImageFile, 0, tags),
             sizeof(nsm->nsm_ImageFile));
    ntf_Copy(nsm->nsm_IconFile, (CONST_STRPTR)GetTagData(APPNOTIFY_IconFile, 0, tags),
             sizeof(nsm->nsm_IconFile));
    ntf_Copy(nsm->nsm_SoundFile, (CONST_STRPTR)GetTagData(APPNOTIFY_SoundFile, 0, tags),
             sizeof(nsm->nsm_SoundFile));
    ntf_Copy(nsm->nsm_BackMsg, withdraw ? withdraw : (CONST_STRPTR)GetTagData(APPNOTIFY_BackMsg, 0, tags),
             sizeof(nsm->nsm_BackMsg));
    nsm->nsm_Withdraw = withdraw ? TRUE : FALSE;
    ntf_Copy(nsm->nsm_Gauges, (CONST_STRPTR)GetTagData(APPNOTIFY_Gauges, 0, tags), sizeof(nsm->nsm_Gauges));

    if (ntf_StartServer())
    {
        if (!(reply = CreateMsgPort()))
            result = APPNOTIFY_ERROR_OUTOFMEM;
        else
        {
            nsm->nsm_Msg.mn_Node.ln_Type = NT_MESSAGE;
            nsm->nsm_Msg.mn_Length = sizeof(*nsm);
            nsm->nsm_Msg.mn_ReplyPort = reply;

            /* The server may be quitting; hold the port still while posting. */
            Forbid();
            if ((server = FindPort(NOTIFYSERVER_PORTNAME)))
                PutMsg(server, &nsm->nsm_Msg);
            Permit();

            if (server)
            {
                WaitPort(reply);
                GetMsg(reply);
                result = nsm->nsm_Result;
            }
            DeleteMsgPort(reply);
        }
    }

    FreeVec(nsm);
    return result;

    AROS_LIBFUNC_EXIT
}
