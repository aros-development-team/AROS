/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: C:Notify - show a notification from the Shell, a script or any
          program that can run a command.
*/

/******************************************************************************

    NAME

        Notify

    SYNOPSIS

        TEXT/A,TITLE/K,APP/K,IMAGE/K,ICON/K,SOUND/K,PRI/K/N,SCREEN/K,URL/K,
        GAUGE/K/M,UPDATE/S,LOGONLY/S,WAIT/S

    LOCATION

        C:

    FUNCTION

        Shows a pop-up notification, through application.library and the
        notification server, and adds it to the notification log.

    INPUTS

        TEXT    -- the message.
        TITLE   -- a heading; the application name if left out.
        APP     -- the application to show it for (default "Shell"). It
                   can be switched off in Prefs/Notifications.
        IMAGE   -- a picture, scaled to 32 x 32.
        ICON    -- a Workbench icon to show instead of a picture.
        SOUND   -- a sound to play.
        PRI     -- 0 to 10. Above 8 the screen flashes; 10 stays until
                   it is closed.
        SCREEN  -- a public screen, or FRONT for the frontmost one.
        URL     -- double-clicking opens this in the browser.
        GAUGE   -- a bar under the text, "<percent>;#RRGGBB;<label>";
                   up to eight, each with its own GAUGE.
        UPDATE  -- take back this application's earlier notifications.
        LOGONLY -- only add it to the log.
        WAIT    -- wait until the pop-up is closed. Return code 0 if it
                   was double-clicked, 5 if it was closed otherwise (with
                   URL the browser takes the double-click, so always 5).
                   CTRL-C takes it back.

    RESULT

        0, 5 (WAIT: closed without a double-click), or 10 with the reason
        printed when it could not be shown.

    EXAMPLE

        Notify "Backup finished" TITLE "Backup" PRI 2
        Notify "Click to read the news" URL https://aros.org WAIT
        Notify "Backing up" GAUGE "40;#3070C0;Work:"

******************************************************************************/

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/application.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/application.h>

#include <string.h>

#define VERSTAG "\0$VER: Notify 1.0 (18.10.2026)"
static const char version[] __attribute__((used)) = VERSTAG;

#define BACKMSG "C:Notify"

enum { A_TEXT, A_TITLE, A_APP, A_IMAGE, A_ICON, A_SOUND, A_PRI, A_SCREEN, A_URL, A_GAUGE, A_UPDATE,
       A_LOGONLY, A_WAIT, A_COUNT };

struct Library *ApplicationBase;

static CONST_STRPTR Reason(ULONG res)
{
    switch (res)
    {
    case APPNOTIFY_ERROR_OUTOFMEM:              return "out of memory";
    case APPNOTIFY_ERROR_TOOMANYMESSAGESQUEUED: return "too many notifications waiting";
    case APPNOTIFY_ERROR_APPNOTALLOWED:         return "switched off for this application in Prefs/Notifications";
    case APPNOTIFY_ERROR_GAMEMODEACTIVE:        return "an application is in game mode";
    case APPNOTIFY_ERROR_TAGS:                  return "nothing to show";
    case APPNOTIFY_ERROR_SERVERNOTRUNNING:      return "SYS:System/Notifications could not be started";
    }
    return "it could not be shown";
}

int main(void)
{
    IPTR args[A_COUNT];
    struct RDArgs *rda;
    struct MsgPort *port = NULL;
    TEXT app[APPNOTIFY_TITLE_MAX], backmsg[512], gauges[512] = "";
    CONST_STRPTR backtag = BACKMSG;
    ULONG appid, res, i;
    LONG pri, rc = RETURN_FAIL;

    memset(args, 0, sizeof(args));
    if (!(rda = ReadArgs("TEXT/A,TITLE/K,APP/K,IMAGE/K,ICON/K,SOUND/K,PRI/K/N,SCREEN/K,URL/K,"
                         "GAUGE/K/M,UPDATE/S,LOGONLY/S,WAIT/S", args, NULL)))
    {
        PrintFault(IoErr(), "Notify");
        return RETURN_FAIL;
    }

    if (!(ApplicationBase = OpenLibrary("application.library", 1)))
    {
        PutStr("Notify: application.library is missing\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    /* Application names have no spaces. */
    strncpy(app, args[A_APP] ? (CONST_STRPTR)args[A_APP] : (CONST_STRPTR)"Shell", sizeof(app) - 1);
    app[sizeof(app) - 1] = '\0';
    for (i = 0; app[i]; i++)
        if (app[i] == ' ')
            app[i] = '_';

    if (args[A_URL])
    {
        strcpy(backmsg, "URL:");
        strncat(backmsg, (STRPTR)args[A_URL], sizeof(backmsg) - 5);
        backtag = backmsg;
    }
    pri = args[A_PRI] ? *(LONG *)args[A_PRI] : 0;
    if (args[A_GAUGE])
    {
        STRPTR *g;

        for (g = (STRPTR *)args[A_GAUGE]; *g; g++)
        {
            if (strlen(gauges) + strlen(*g) + 2 >= sizeof(gauges))
                break;
            if (gauges[0])
                strcat(gauges, "\n");
            strcat(gauges, *g);
        }
    }

    if (!(appid = RegisterApplication(app, REGAPP_NoIcon, TRUE, TAG_DONE)))
    {
        PutStr("Notify: cannot register the application\n");
        goto out;
    }
    GetApplicationAttrs(appid, APPATTR_Port, (IPTR)&port, TAG_DONE);

    res = Notify(appid,
                 APPNOTIFY_Text,          args[A_TEXT],
                 args[A_TITLE] ? APPNOTIFY_Title : TAG_IGNORE, args[A_TITLE],
                 args[A_IMAGE] ? APPNOTIFY_ImageFile : TAG_IGNORE, args[A_IMAGE],
                 args[A_ICON] ? APPNOTIFY_IconFile : TAG_IGNORE, args[A_ICON],
                 args[A_SOUND] ? APPNOTIFY_SoundFile : TAG_IGNORE, args[A_SOUND],
                 args[A_SCREEN] ? APPNOTIFY_PubScreenName : TAG_IGNORE, args[A_SCREEN],
                 gauges[0] ? APPNOTIFY_Gauges : TAG_IGNORE, (IPTR)gauges,
                 APPNOTIFY_Pri,           pri < 0 ? 0 : pri,
                 APPNOTIFY_Update,        args[A_UPDATE] != 0,
                 APPNOTIFY_LogOnly,       args[A_LOGONLY] != 0,
                 APPNOTIFY_BackMsg,       (IPTR)backtag,
                 APPNOTIFY_CloseOnDC,     TRUE,
                 APPNOTIFY_ConfirmClose,  args[A_WAIT] != 0,
                 TAG_DONE);

    if (res != APPNOTIFY_OK_MSGQUEUED)
    {
        Printf("Notify: %s\n", (IPTR)Reason(res));
        goto out;
    }
    rc = RETURN_OK;

    if (args[A_WAIT] && !args[A_LOGONLY] && port)
    {
        BOOL clicked = FALSE, closed = FALSE;

        while (!closed)
        {
            struct ApplicationMsg *am;
            ULONG sigs = Wait((1UL << port->mp_SigBit) | SIGBREAKF_CTRL_C);

            while ((am = (struct ApplicationMsg *)GetMsg(port)))
            {
                if (am->type == APPLIBMT_CustomMsg)
                    clicked = TRUE;
                else if (am->type == APPLIBMT_ConfirmClose)
                    closed = TRUE;
                ReplyMsg(&am->msg);
            }
            if (sigs & SIGBREAKF_CTRL_C)
            {
                Notify(appid, APPNOTIFY_Withdraw, (IPTR)backtag, TAG_DONE);
                PrintFault(ERROR_BREAK, "Notify");
                rc = RETURN_WARN;
                break;
            }
        }
        if (closed)
            rc = clicked ? RETURN_OK : RETURN_WARN;
    }

out:
    if (appid)
        UnregisterApplication(appid, NULL);
    CloseLibrary(ApplicationBase);
    FreeArgs(rda);
    return rc;
}
