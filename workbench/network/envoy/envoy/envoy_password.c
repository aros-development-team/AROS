/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library - PasswordRequestA(): a new password, typed twice,
          optionally after the old one (re/spec/envoy-library.md §6).
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <libraries/mui.h>
#include <exec/memory.h>
#include <string.h>

#include "envoy_intern.h"

struct PwReq
{
    STRPTR  Old;            /* password to check, or buffer when OldLen is set */
    ULONG   OldLen;
    STRPTR  NewBuff;
    ULONG   NewLen;
    BOOL    Required;
    BOOL    HasOld;
    char    TypedOld[128], New[32], Repeat[32];
};

static const struct ReqTagMap pwmap[] =
{
    { PWREQ_Screen, RQS_SCREEN }, { PWREQ_Window, RQS_WINDOW }, { PWREQ_OptimWindow, RQS_OPTIM },
    { PWREQ_Title, RQS_TITLE }, { PWREQ_CallBack, RQS_CALLBACK }, { PWREQ_MsgPort, RQS_MSGPORT }, { 0, 0 }
};

static BOOL Acceptable(struct Req *req, struct PwReq *pw)
{
    ReqGetString(req, 0, pw->TypedOld, sizeof(pw->TypedOld));
    ReqGetString(req, 1, pw->New, sizeof(pw->New));
    ReqGetString(req, 2, pw->Repeat, sizeof(pw->Repeat));

    if (pw->HasOld && !pw->OldLen && strcmp(pw->TypedOld, pw->Old))
        return FALSE;
    if (strcmp(pw->New, pw->Repeat))
        return FALSE;
    if (pw->Required)
    {
        CONST_STRPTR old = pw->HasOld ? (CONST_STRPTR)(pw->OldLen ? (STRPTR)pw->TypedOld : pw->Old) : NULL;
        if (strlen(pw->New) < 6)
            return FALSE;
        if (old && !strcmp(pw->New, old))
            return FALSE;
    }
    return TRUE;
}

static void Validate(struct Req *req, struct PwReq *pw)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    SetAttrs(req->OK, MUIA_Disabled, !Acceptable(req, pw), TAG_DONE);
}

static BOOL PwSetup(struct Req *req)
{
    struct EnvoyBase *EnvoyBase = req->EnvoyBase;
    struct PwReq *pw = req->ClientData;
    struct TagItem *tag, *tstate = req->Tags;

    while ((tag = NextTagItem(&tstate)))
    {
        switch (tag->ti_Tag)
        {
        case PWREQ_OldPassword:  pw->Old = (STRPTR)tag->ti_Data; break;
        case PWREQ_OldPWLen:     pw->OldLen = tag->ti_Data; break;
        case PWREQ_NewPWBuff:    pw->NewBuff = (STRPTR)tag->ti_Data; break;
        case PWREQ_NewPWBuffLen: pw->NewLen = tag->ti_Data; break;
        case PWREQ_PWRequired:   pw->Required = (tag->ti_Data & 0xFFFF) != 0; break;
        }
    }
    /* an empty old password means there is none to ask for */
    pw->HasOld = pw->Old && (pw->OldLen || pw->Old[0]);
    if (!pw->HasOld)
    {
        SetAttrs(req->Lbl[0], MUIA_ShowMe, FALSE, TAG_DONE);
        SetAttrs(req->Str[0], MUIA_ShowMe, FALSE, TAG_DONE);
    }
    Validate(req, pw);
    ReqActivate(req, pw->HasOld ? 0 : 1);
    return TRUE;
}

static BOOL PwEvent(struct Req *req, ULONG id)
{
    struct PwReq *pw = req->ClientData;

    switch (id)
    {
    case RQID_CHANGED:
        Validate(req, pw);
        return TRUE;
    case RQID_STRING1:
        ReqActivate(req, 1);
        return TRUE;
    case RQID_STRING2:
        ReqActivate(req, 2);
        return TRUE;
    case RQID_STRING3:
    case RQID_OK:
        if (!Acceptable(req, pw))
            return TRUE;
        if (pw->HasOld && pw->OldLen)
            ReqCopyOut(pw->Old, pw->OldLen, pw->TypedOld);
        ReqCopyOut(pw->NewBuff, pw->NewLen, pw->New);
        req->Result = TRUE;
        return FALSE;
    }
    return TRUE;
}

static const struct ReqClient pwclient =
{
    MSG_ELIB_PASSWORD_REQUEST, { MSG_ELIB_OLDPASSWORD, MSG_ELIB_PASSWORD1, MSG_ELIB_PASSWORD2 }, 0, pwmap,
    RQF_NOLIST | RQF_MASK1 | RQF_MASK23 | RQF_WATCH,
    PwSetup, NULL, PwEvent, NULL, NULL
};

/*****************************************************************************

    NAME */
#include <proto/envoy.h>

        AROS_LH1(BOOL, PasswordRequestA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct EnvoyBase *, EnvoyBase, 19, Envoy)

/*  FUNCTION
        Asks for a new password, typed twice. With PWREQ_OldPassword the
        old one has to be typed as well; it is checked here unless
        PWREQ_OldPWLen makes that tag a buffer for the caller to check.
        The OK button is enabled only while the entries are acceptable.

    INPUTS
        tags - PWREQ_#? tags.

    RESULT
        TRUE with the new password in PWREQ_NewPWBuff (and the typed old
        one in PWREQ_OldPassword when PWREQ_OldPWLen is set), FALSE on
        Cancel or failure.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PwReq *pw;
    BOOL ok;

    if (!(pw = AllocVec(sizeof(struct PwReq), MEMF_CLEAR | MEMF_PUBLIC)))
        return FALSE;
    ok = ReqRun(EnvoyBase, &pwclient, pw, tags);
    FreeVec(pw);
    return ok;

    AROS_LIBFUNC_EXIT
}
