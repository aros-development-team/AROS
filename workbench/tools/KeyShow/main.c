/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/icon.h>
#include <proto/alib.h>
#include <proto/commodities.h>
#include <proto/utility.h>

#include <libraries/mui.h>

#include <strings.h>

#include "keyboardgroup_class.h"
#include "locale.h"

//#define DEBUG 1
#include <aros/debug.h>

static void cleanup_exit(CONST_STRPTR str);
static void HandleAll(void);

static Object *app, *win;
static BOOL cx_popup = FALSE;
static struct Hook broker_hook;
static struct Hook show_hook;

AROS_UFH3(void, broker_func,
    AROS_UFHA(struct Hook *, hook,   A0),
    AROS_UFHA(Object *     , obj,    A2),
    AROS_UFHA(CxMsg *      , msg,    A1))
{
    AROS_USERFUNC_INIT

    D(bug("KeyShow: Broker hook called\n"));
    if (CxMsgType(msg) == CXM_COMMAND)
    {
        if (CxMsgID(msg) == CXCMD_APPEAR)
        {
            CallHookPkt(&show_hook, NULL, NULL);
        }
        else if (CxMsgID(msg) == CXCMD_DISAPPEAR)
        {
            set(win, MUIA_Window_Open, FALSE);
        }
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(void, show_func,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR,             obj,    A2),
    AROS_UFHA(APTR,             param,  A1))
{
    AROS_USERFUNC_INIT

    if (XGET(app, MUIA_Application_Iconified) == TRUE)
        set(app, MUIA_Application_Iconified, FALSE);
    else
        set(win, MUIA_Window_Open, TRUE);

    AROS_USERFUNC_EXIT
}

int main(int argc, char **argv)
{
    ULONG kbtype = MUIV_KeyboardGroup_Type_Amiga;
    STRPTR result;

    // Read icon even if we have been started from CLI
    struct DiskObject *dobj = GetDiskObject("PROGDIR:KeyShow");
    if (dobj)
    {
        STRPTR *toolarray = dobj->do_ToolTypes;

        result = FindToolType(toolarray, "TYPE");
        if (result)
        {
            if (strcasecmp(result, "PC105") == 0)
            {
                kbtype = MUIV_KeyboardGroup_Type_PC105;
            }
            else if (strcasecmp(result, "PC104") == 0)
            {
                kbtype = MUIV_KeyboardGroup_Type_PC104;
            }
        }
    }

    broker_hook.h_Entry = HookEntry;
    broker_hook.h_SubEntry = (HOOKFUNC)broker_func;
    show_hook.h_Entry = (HOOKFUNC)show_func;

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR)"KeyShow",
        MUIA_Application_Version, (IPTR)"$VER: KeyShow 1.3 (06.03.2012)",
        MUIA_Application_Copyright, (IPTR)_(MSG_AppCopyright),
        MUIA_Application_Author, (IPTR)"The AROS Development Team",
        MUIA_Application_Description, (IPTR)_(MSG_AppDescription),
        MUIA_Application_Base, (IPTR)"KEYSHOW",
        MUIA_Application_DiskObject, (IPTR)dobj,
        MUIA_Application_BrokerHook, (IPTR)&broker_hook,
        SubWindow, (IPTR)(win = WindowObject,
            MUIA_Window_Title, (IPTR)_(MSG_WI_TITLE),
            MUIA_Window_ID, MAKE_ID('K','S','W','N'),
            WindowContents, (IPTR)KeyboardGroupObject,
                MUIA_KeyboardGroup_Type, kbtype,
            End,
        End),
    End;

    if (app == NULL)
    {
        cleanup_exit("Can't create application");
    }

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    cx_popup = TRUE;
    HandleAll();
    FreeDiskObject(dobj);
    cleanup_exit(NULL);
    return 0;
}

static void HandleAll(void)
{
    ULONG sigs = 0;

    set(win, MUIA_Window_Open, cx_popup);
    
    while((LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
           != MUIV_Application_ReturnID_Quit)
    {
        if (sigs)
        {
            sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
            if (sigs & SIGBREAKF_CTRL_C)
            {
                break;
            }
            if (sigs & SIGBREAKF_CTRL_F)
            {
                CallHookPkt(&show_hook, NULL, NULL);
            }
        }
    }
}

static void cleanup_exit(CONST_STRPTR str)
{
    if (str)
    {
        struct EasyStruct es =
        {
            sizeof(struct EasyStruct), 0,
            _(MSG_ERR), str, _(MSG_OK)
        };
        EasyRequestArgs(NULL, &es, NULL, NULL);
    }
    MUI_DisposeObject(app);
}
