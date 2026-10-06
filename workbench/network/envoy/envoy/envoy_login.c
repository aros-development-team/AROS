/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library - LoginRequestA(): a name and a password
          (re/spec/envoy-library.md §4). No verification happens here.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <dos/var.h>
#include <string.h>

#include "envoy_intern.h"

struct LoginReq
{
    STRPTR  NameBuff, PassBuff;
    ULONG   NameLen, PassLen;
    char    Name[32], Pass[32];
};

static const struct ReqTagMap loginmap[] =
{
    { LREQ_Screen, RQS_SCREEN }, { LREQ_Window, RQS_WINDOW }, { LREQ_OptimWindow, RQS_OPTIM },
    { LREQ_Title, RQS_TITLE }, { LREQ_CallBack, RQS_CALLBACK }, { LREQ_MsgPort, RQS_MSGPORT }, { 0, 0 }
};

static BOOL LoginSetup(struct Req *req)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct LoginReq *lr = req->ClientData;
    struct TagItem *tag, *tstate = req->Tags;

    if (GetVar("USERNAME", lr->Name, sizeof(lr->Name), GVF_GLOBAL_ONLY) <= 0)
        lr->Name[0] = '\0';
    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case LREQ_NameBuff:    lr->NameBuff = (STRPTR)tag->ti_Data; break;
        case LREQ_NameBuffLen: lr->NameLen = tag->ti_Data; break;
        case LREQ_PassBuff:    lr->PassBuff = (STRPTR)tag->ti_Data; break;
        case LREQ_PassBuffLen: lr->PassLen = tag->ti_Data; break;
        case LREQ_UserName:
            if (tag->ti_Data)
                ReqCopyOut(lr->Name, sizeof(lr->Name), (CONST_STRPTR)tag->ti_Data);
            break;
        case LREQ_Password:
            if (tag->ti_Data)
                ReqCopyOut(lr->Pass, sizeof(lr->Pass), (CONST_STRPTR)tag->ti_Data);
            break;
        }
    }
    ReqSetString(req, 0, lr->Name);
    ReqSetString(req, 1, lr->Pass);
    ReqActivate(req, (lr->Name[0] && !lr->Pass[0]) ? 1 : 0);
    return TRUE;
}

static BOOL LoginEvent(struct Req *req, ULONG id)
{
    struct LoginReq *lr = req->ClientData;

    switch (id)
    {
    case RQID_STRING1:
        ReqActivate(req, 1);
        return TRUE;
    case RQID_STRING2:
    case RQID_OK:
        ReqGetString(req, 0, lr->Name, sizeof(lr->Name));
        ReqGetString(req, 1, lr->Pass, sizeof(lr->Pass));
        ReqCopyOut(lr->NameBuff, lr->NameLen, lr->Name);
        ReqCopyOut(lr->PassBuff, lr->PassLen, lr->Pass);
        req->Result = TRUE;
        return FALSE;
    }
    return TRUE;
}

static const struct ReqClient loginclient =
{
    MSG_ELIB_LOGIN_REQUEST, { MSG_ELIB_USERNAME, MSG_ELIB_PASSWORD, 0 }, 0, loginmap, RQF_NOLIST | RQF_MASK23,
    LoginSetup, NULL, LoginEvent, NULL, NULL
};

/*****************************************************************************

    NAME */
#include <proto/envoy.h>

        AROS_LH1(BOOL, LoginRequestA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct EnvoyBase *, EnvoyBase, 6, Envoy)

/*  FUNCTION
        Asks for a user name and a password. The name is preset from
        LREQ_UserName or, failing that, the global variable USERNAME.

    INPUTS
        tags - LREQ_#? tags.

    RESULT
        TRUE with the name in LREQ_NameBuff and the password in
        LREQ_PassBuff (each NUL-terminated within its length), FALSE on
        Cancel or failure. Empty entries are accepted; checking them is the
        caller's business.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct LoginReq *lr;
    BOOL ok;

    if (!(lr = AllocVec(sizeof(struct LoginReq), MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    ok = ReqRun(EnvoyBase, &loginclient, lr, tags);
    FreeVec(lr);
    return ok;

    AROS_LIBFUNC_EXIT
}
