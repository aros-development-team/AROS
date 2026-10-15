/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - running a stack, and the six application calls
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/pam.h>
#include <resources/log.h>
#include <string.h>

#include "pam_intern.h"

static LONG CallEntry(struct PamBase *PamBase, struct PamHandle *h, struct PamEntry *e, ULONG op, ULONG flags)
{
    if (e->Builtin)
    {
        if (!e->Builtin->Func[op])
            return PAM_IGNORE;
        return e->Builtin->Func[op](PamBase, h, flags, e->Argc, e->Argv);
    }
    switch (op)
    {
    case PAM_OP_AUTHENTICATE:   return PamModuleCall(PAMM_LVO_Authenticate, e->Module->Base, h, flags, e->Argc, e->Argv);
    case PAM_OP_SETCRED:        return PamModuleCall(PAMM_LVO_SetCred,      e->Module->Base, h, flags, e->Argc, e->Argv);
    case PAM_OP_ACCT_MGMT:      return PamModuleCall(PAMM_LVO_AcctMgmt,     e->Module->Base, h, flags, e->Argc, e->Argv);
    case PAM_OP_OPEN_SESSION:   return PamModuleCall(PAMM_LVO_OpenSession,  e->Module->Base, h, flags, e->Argc, e->Argv);
    case PAM_OP_CLOSE_SESSION:  return PamModuleCall(PAMM_LVO_CloseSession, e->Module->Base, h, flags, e->Argc, e->Argv);
    case PAM_OP_CHAUTHTOK:      return PamModuleCall(PAMM_LVO_ChAuthTok,    e->Module->Base, h, flags, e->Argc, e->Argv);
    }
    return PAM_IGNORE;
}

/*
 * The control words, with the usual PAM meaning:
 *   required   - a failure is remembered (the first one wins) but the rest
 *                of the stack still runs, so a caller cannot tell which
 *                module refused;
 *   requisite  - a failure ends the stack at once;
 *   sufficient - a success ends the stack: with success, or with the
 *                failure a required module gave before it; a failure is
 *                ignored. PAM_NEW_AUTHTOK_REQD ends it too;
 *   optional   - counts only if nothing else in the stack gave a result.
 * PAM_NEW_AUTHTOK_REQD from a required or requisite module lets the stack
 * go on and is the result if nothing fails. An empty stack, or one in
 * which nothing answered, denies.
 *
 * replay: only the entries that ran in the pass the stack is frozen at
 * (PamSetCred() after PamAuthenticate(), the update pass of
 * PamChAuthTok() after the preliminary one); record: mark the entries
 * that run now.
 */
static LONG RunEntries(struct PamBase *PamBase, struct PamHandle *h, ULONG group, ULONG op, ULONG flags, BOOL replay, BOOL record)
{
    struct PamEntry *e;
    LONG required_fail = PAM_SUCCESS;
    LONG optional_res = PAM_IGNORE;
    BOOL any = FALSE, newtok = FALSE;
    LONG r;

    if (record)
    {
        ForeachNode(&h->Stack[group], e)
            e->Ran = FALSE;
    }

    ForeachNode(&h->Stack[group], e)
    {
        if (replay && !e->Ran)
            continue;
        if (record)
            e->Ran = TRUE;
        r = CallEntry(PamBase, h, e, op, flags);
        D(bug(DEBUG_NAME_STR " %s: %s %s -> %ld\n", __func__, e->Builtin ? e->Builtin->Name : e->Module->Name, (char *[]){"required","requisite","sufficient","optional"}[e->Control], (long)r);)
        if (r == PAM_IGNORE)
            continue;
        switch (e->Control)
        {
        case PAM_CTL_REQUIRED:
        case PAM_CTL_REQUISITE:
            any = TRUE;
            if (r == PAM_NEW_AUTHTOK_REQD)
                newtok = TRUE;
            else if (r != PAM_SUCCESS)
            {
                if (required_fail == PAM_SUCCESS)
                    required_fail = r;
                if (e->Control == PAM_CTL_REQUISITE)
                    return required_fail;
            }
            break;
        case PAM_CTL_SUFFICIENT:
            if (r == PAM_SUCCESS || r == PAM_NEW_AUTHTOK_REQD)
            {
                if (required_fail != PAM_SUCCESS)
                    return required_fail;
                return (r == PAM_NEW_AUTHTOK_REQD || newtok) ? PAM_NEW_AUTHTOK_REQD : PAM_SUCCESS;
            }
            break;
        case PAM_CTL_OPTIONAL:
            if (optional_res == PAM_IGNORE)
                optional_res = r;
            break;
        }
    }
    if (required_fail != PAM_SUCCESS)
        return required_fail;
    if (any)
        return newtok ? PAM_NEW_AUTHTOK_REQD : PAM_SUCCESS;
    if (optional_res != PAM_IGNORE)
        return optional_res;
    return (op == PAM_OP_AUTHENTICATE) ? PAM_AUTH_ERR : PAM_PERM_DENIED;
}

LONG PamRunStack(struct PamBase *PamBase, struct PamHandle *h, ULONG group, ULONG op, ULONG flags)
{
    BOOL replay = FALSE, record = FALSE;
    LONG r;

    if (!h)
        return PAM_SYSTEM_ERR;
    /* A service change made during an earlier run takes effect now */
    if (h->ReloadPending && !h->Running)
    {
        PamFreeStack(PamBase, h);
        h->ReloadPending = FALSE;
        h->Authenticated = h->PrelimDone = FALSE;
    }
    if (!PamLoadStack(PamBase, h))
        return PAM_SERVICE_ERR;

    switch (op)
    {
    case PAM_OP_AUTHENTICATE:
        record = TRUE;
        break;
    case PAM_OP_SETCRED:
        replay = h->Authenticated;
        break;
    case PAM_OP_CHAUTHTOK:
        if (flags & PAM_PRELIM_CHECK)
            record = TRUE;
        else
            replay = h->PrelimDone;
        break;
    }

    h->Running++;
    r = RunEntries(PamBase, h, group, op, flags, replay, record);
    h->Running--;

    if (op == PAM_OP_AUTHENTICATE)
        h->Authenticated = (r == PAM_SUCCESS || r == PAM_NEW_AUTHTOK_REQD);
    else if (op == PAM_OP_CHAUTHTOK && (flags & PAM_PRELIM_CHECK))
        h->PrelimDone = (r == PAM_SUCCESS);
    return r;
}

static void ApplyFailDelay(struct PamBase *PamBase, struct PamHandle *h)
{
    if (h->FailDelay && h->Interactive)
    {
        ULONG ticks = (h->FailDelay + 19999) / 20000;   /* 50 ticks per second */
        Delay(ticks ? ticks : 1);
    }
    h->FailDelay = 0;
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamAuthenticate,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 10, Pam)

/*  FUNCTION
        Run the service's "auth" stack: establish that the user is who they
        claim to be. The modules obtain the user name and the password
        through the conversation unless both are already set.

    INPUTS
        flags - PAM_SILENT, PAM_DISALLOW_NULL_AUTHTOK.

    RESULT
        PAM_SUCCESS or a failure code; PAM_AUTH_ERR for a wrong password or
        unknown user, PAM_MAXTRIES when locked out.

    NOTES
        Nothing about the calling task changes; see PamSetCred().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG r;

    if (!handle)
        return PAM_SYSTEM_ERR;
    r = PamRunStack(PamBase, handle, PAM_GROUP_AUTH, PAM_OP_AUTHENTICATE, flags);
    if (r != PAM_SUCCESS && r != PAM_NEW_AUTHTOK_REQD)
    {
        ApplyFailDelay(PamBase, handle);
        PamLogA(PamBase, handle, LOGF_Flag_Type_Warn, "authentication of %s failed: %s",
                (IPTR []){ (IPTR)(handle->User ? handle->User : (STRPTR)"?"), (IPTR)PamStrError(handle, r) });
    }
    else
        handle->FailDelay = 0;
    /* the token now belongs to the authentication; a password change on
     * this handle must not take it for the new one */
    handle->AuthTokFromAuth = (handle->AuthTok != NULL);
    handle->LastStatus = r;
    return r;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamSetCred,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(LONG, flags, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 11, Pam)

/*  FUNCTION
        Establish (PAM_ESTABLISH_CRED) or drop (PAM_DELETE_CRED) the
        credentials of the authenticated user on the task of PAM_AROS_TASK.
        With security.library this changes the task's owner. Call after a
        successful PamAuthenticate().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG r;

    if (!handle)
        return PAM_SYSTEM_ERR;
    r = PamRunStack(PamBase, handle, PAM_GROUP_AUTH, PAM_OP_SETCRED, flags);
    handle->LastStatus = r;
    return r;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamAcctMgmt,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 12, Pam)

/*  FUNCTION
        Run the "account" stack: may this user use this service now, from
        here. Call after PamAuthenticate().

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG r;

    if (!handle)
        return PAM_SYSTEM_ERR;
    r = PamRunStack(PamBase, handle, PAM_GROUP_ACCOUNT, PAM_OP_ACCT_MGMT, flags);
    handle->LastStatus = r;
    return r;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamOpenSession,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 13, Pam)

/*  FUNCTION
        Run the "session" stack's open functions (records, variables,
        assigns, profile).

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG r;

    if (!handle)
        return PAM_SYSTEM_ERR;
    r = PamRunStack(PamBase, handle, PAM_GROUP_SESSION, PAM_OP_OPEN_SESSION, flags);
    handle->LastStatus = r;
    return r;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamCloseSession,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 14, Pam)

/*  FUNCTION
        Run the "session" stack's close functions.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG r;

    if (!handle)
        return PAM_SYSTEM_ERR;
    r = PamRunStack(PamBase, handle, PAM_GROUP_SESSION, PAM_OP_CLOSE_SESSION, flags);
    handle->LastStatus = r;
    return r;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamChAuthTok,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 15, Pam)

/*  FUNCTION
        Change the user's password: the "password" stack is run twice, with
        PAM_PRELIM_CHECK (may it be changed, is the old one right) and then
        with PAM_UPDATE_AUTHTOK, which replays the modules of the first
        pass. Applications may not pass either flag themselves. A token
        that was used to authenticate on the same handle is taken as the
        old password; the new one is asked for twice when the
        conversation is interactive.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;
    LONG r;

    if (!h)
        return PAM_SYSTEM_ERR;
    /* The passes are the library's business */
    if (flags & (PAM_PRELIM_CHECK | PAM_UPDATE_AUTHTOK))
        return PAM_SYSTEM_ERR;

    /* A token left from authentication is the old password, never the new */
    if (h->AuthTokFromAuth)
    {
        if (!h->OldAuthTok)
        {
            h->OldAuthTok = h->AuthTok;
            h->AuthTok = NULL;
        }
        else
            PamSetString(PamBase, &h->AuthTok, NULL, TRUE);
        h->AuthTokFromAuth = FALSE;
    }

    r = PamRunStack(PamBase, h, PAM_GROUP_PASSWORD, PAM_OP_CHAUTHTOK, flags | PAM_PRELIM_CHECK);
    if (r == PAM_SUCCESS)
    {
        h->InUpdate = TRUE;
        r = PamRunStack(PamBase, h, PAM_GROUP_PASSWORD, PAM_OP_CHAUTHTOK, flags | PAM_UPDATE_AUTHTOK);
        h->InUpdate = FALSE;
    }
    PamLogA(PamBase, h, (r == PAM_SUCCESS) ? LOGF_Flag_Type_Information : LOGF_Flag_Type_Warn,
            "password change for %s: %s", (IPTR []){ (IPTR)(h->User ? h->User : (STRPTR)"?"), (IPTR)PamStrError(h, r) });
    h->LastStatus = r;
    return r;

    AROS_LIBFUNC_EXIT
}
