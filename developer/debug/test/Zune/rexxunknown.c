/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    An application with an ARexx port but no command table gets a command it
    did not define: without MUIA_Application_RexxHook it is answered with
    RC_ERROR, with the hook the hook's return value and the result string
    set through MUIA_Application_RexxString come back.
*/

#include <exec/ports.h>
#include <libraries/mui.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/muimaster.h>
#include <proto/rexxsyslib.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>

struct RxsLib *RexxSysBase;

static int failures;
static int hookcalls, hooksaw;

static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

AROS_UFH3(static IPTR, RexxHook,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, app, A2),
    AROS_UFHA(struct RexxMsg *, msg, A1))
{
    AROS_USERFUNC_INIT

    hookcalls++;
    hooksaw = strcmp((const char *)msg->rm_Args[0], "NOSUCHCOMMAND") == 0;
    set(app, MUIA_Application_RexxString, (IPTR)"handled by hook");
    return 7;

    AROS_USERFUNC_EXIT
}

/* send "NOSUCHCOMMAND" to the application's port and pump it until the
   message comes back; the caller clears and deletes the returned message */
static struct RexxMsg *ask(Object *app, struct MsgPort *reply, ULONG action)
{
    struct MsgPort *port;
    struct RexxMsg *rmsg, *back = NULL;
    ULONG sigs = 0, replymask = 1UL << reply->mp_SigBit;

    if (!(rmsg = CreateRexxMsg(reply, NULL, (UBYTE *)"REXXUNKNOWN.1")))
        return NULL;
    rmsg->rm_Action = action;
    if (!(rmsg->rm_Args[0] = (IPTR)CreateArgstring((CONST_STRPTR)"NOSUCHCOMMAND", 13)))
    {
        DeleteRexxMsg(rmsg);
        return NULL;
    }

    Forbid();
    if ((port = FindPort((CONST_STRPTR)"REXXUNKNOWN.1")))
        PutMsg(port, (struct Message *)rmsg);
    Permit();
    if (!port)
    {
        printf("FAIL: the application has no REXXUNKNOWN.1 port\n");
        ClearRexxMsg(rmsg, 1);
        DeleteRexxMsg(rmsg);
        return NULL;
    }

    while (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit)
    {
        if ((back = (struct RexxMsg *)GetMsg(reply)))
            break;
        if (sigs)
        {
            sigs = Wait(sigs | replymask | SIGBREAKF_CTRL_C);
            if (sigs & SIGBREAKF_CTRL_C)
                break;
            if (sigs & replymask)
                sigs &= ~replymask;
        }
    }
    if (!back)
        back = (struct RexxMsg *)GetMsg(reply);
    if (back != rmsg)
    {
        printf("FAIL: %s\n", back ? "a different message came back" : "no reply");
        failures++;
        return NULL;
    }
    return back;
}

static void done(struct RexxMsg *rmsg)
{
    if (rmsg)
    {
        ClearRexxMsg(rmsg, 1);
        DeleteRexxMsg(rmsg);
    }
}

int main(void)
{
    Object *app;
    struct MsgPort *reply;
    struct RexxMsg *back;
    struct Hook hook;

    if (!(RexxSysBase = (struct RxsLib *)OpenLibrary((CONST_STRPTR)"rexxsyslib.library", 0)))
    {
        printf("FAIL: no rexxsyslib.library\n");
        return RETURN_FAIL;
    }
    if (!(reply = CreateMsgPort()))
    {
        CloseLibrary((struct Library *)RexxSysBase);
        return RETURN_FAIL;
    }

    /* no command table, no hook */
    app = ApplicationObject,
        MUIA_Application_Title,   (IPTR)"rexxunknown",
        MUIA_Application_Base,    (IPTR)"REXXUNKNOWN",
        MUIA_Application_UseRexx, TRUE,
    End;
    check(app != NULL, "application without a command table");
    if (app)
    {
        back = ask(app, reply, RXCOMM);
        check(back && back->rm_Result1 == RC_ERROR, "unknown command answered with RC_ERROR");
        done(back);
        MUI_DisposeObject(app);
    }

    /* no command table, a RexxHook */
    memset(&hook, 0, sizeof(hook));
    hook.h_Entry = (HOOKFUNC)RexxHook;
    app = ApplicationObject,
        MUIA_Application_Title,    (IPTR)"rexxunknown",
        MUIA_Application_Base,     (IPTR)"REXXUNKNOWN",
        MUIA_Application_UseRexx,  TRUE,
        MUIA_Application_RexxHook, (IPTR)&hook,
    End;
    check(app != NULL, "application with a RexxHook");
    if (app)
    {
        back = ask(app, reply, RXCOMM | RXFF_RESULT);
        check(hookcalls == 1, "the hook was called once");
        check(hooksaw, "the hook saw the command");
        check(back && back->rm_Result1 == 7, "the hook's return value is the result code");
        check(back && back->rm_Result2 && strcmp((const char *)back->rm_Result2, "handled by hook") == 0,
              "MUIA_Application_RexxString is the result string");
        if (back && back->rm_Result2)
            DeleteArgstring((UBYTE *)back->rm_Result2);
        done(back);
        MUI_DisposeObject(app);
    }

    DeleteMsgPort(reply);
    CloseLibrary((struct Library *)RexxSysBase);
    printf("REXXUNKNOWN %s\n", failures ? "FAIL" : "PASS");
    return failures ? RETURN_WARN : RETURN_OK;
}
