#ifndef APPLICATION_SERVER_H
#define APPLICATION_SERVER_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Request format between application.library and the notification
          server (SYS:System/Notifications). Private to the two of them.
*/

#include <exec/ports.h>
#include <libraries/application.h>

#define NOTIFYSERVER_PORTNAME   "AROS.Notifications"
#define NOTIFYSERVER_PATH       "SYS:System/Notifications"
#define NOTIFYSERVER_VERSION    3

#define NOTIFYSERVER_SCREENMAX  128
#define NOTIFYSERVER_PATHMAX    512
#define NOTIFYSERVER_BACKMAX    512
#define NOTIFYSERVER_GAUGESMAX  512

struct NotifyServerMsg
{
    struct Message  nsm_Msg;
    ULONG           nsm_Version;
    ULONG           nsm_AppID;
    ULONG           nsm_Result;     /* set by the server: APPNOTIFY_ result */
    BOOL            nsm_CloseOnDC;
    BOOL            nsm_Update;
    BOOL            nsm_LogOnly;
    BOOL            nsm_ConfirmClose;
    BOOL            nsm_Withdraw;   /* nsm_BackMsg: take those back */
    UBYTE           nsm_ImageVertAlign;
    UBYTE           nsm_Pri;
    TEXT            nsm_AppName[APPNOTIFY_TITLE_MAX + 1];
    TEXT            nsm_Title[APPNOTIFY_TITLE_MAX + 1];
    TEXT            nsm_Text[APPNOTIFY_TEXT_MAX + 1];
    TEXT            nsm_PubScreen[NOTIFYSERVER_SCREENMAX];
    TEXT            nsm_ImageFile[NOTIFYSERVER_PATHMAX];
    TEXT            nsm_IconFile[NOTIFYSERVER_PATHMAX];
    TEXT            nsm_SoundFile[NOTIFYSERVER_PATHMAX];
    TEXT            nsm_BackMsg[NOTIFYSERVER_BACKMAX];
    TEXT            nsm_Gauges[NOTIFYSERVER_GAUGESMAX];
};

#endif /* APPLICATION_SERVER_H */
