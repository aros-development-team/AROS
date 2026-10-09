/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - running a stack, and the six application calls
*/

#include <proto/exec.h>
#include <proto/dos.h>
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
 * The simple PAM control words:
 *   required   - a failure is remembered (first one wins) but the rest of
 *                the stack still runs, so a caller cannot tell which module
 *                refused;
 *   requisite  - a failure ends the stack at once;
 *   sufficient - a success ends the stack with success unless a required
 *                module failed earlier; a failure is ignored;
 *   optional   - counts only if nothing else in the stack gave a result.
 * An empty stack, or one in which nothing answered, denies.
 */
LONG PamRunStack(struct PamBase *PamBase, struct PamHandle *h, ULONG group, ULONG op, ULONG flags)
{
    struct PamEntry *e;
    LONG required_fail = PAM_SUCCESS;
    LONG optional_res = PAM_IGNORE;
    BOOL any = FALSE;
    LONG r;

    if (!h)
        return PAM_SYSTEM_ERR;
    if (!PamLoadStack(PamBase, h))
        return PAM_SERVICE_ERR;

    ForeachNode(&h->Stack[group], e)
    {
        r = CallEntry(PamBase, h, e, op, flags);
        D(bug(DEBUG_NAME_STR " %s: %s %s -> %ld\n", __func__, e->Builtin ? e->Builtin->Name : e->Module->Name, (char *[]){"required","requisite","sufficient","optional"}[e->Control], (long)r);)
        if (r == PAM_IGNORE)
            continue;
        switch (e->Control)
        {
        case PAM_CTL_REQUIRED:
            any = TRUE;
            if (r != PAM_SUCCESS && required_fail == PAM_SUCCESS)
                required_fail = r;
            break;
        case PAM_CTL_REQUISITE:
            any = TRUE;
            if (r != PAM_SUCCESS)
                return (required_fail != PAM_SUCCESS) ? required_fail : r;
            break;
        case PAM_CTL_SUFFICIENT:
            if (r == PAM_SUCCESS)
            {
                if (required_fail == PAM_SUCCESS)
                    return PAM_SUCCESS;
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
        return PAM_SUCCESS;
    if (optional_res != PAM_IGNORE)
        return optional_res;
    return (op == PAM_OP_AUTHENTICATE) ? PAM_AUTH_ERR : PAM_PERM_DENIED;
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

    LONG r = PamRunStack(PamBase, handle, PAM_GROUP_AUTH, PAM_OP_AUTHENTICATE, flags);

    if (r != PAM_SUCCESS)
        ApplyFailDelay(PamBase, handle);
    else
        handle->FailDelay = 0;
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

    LONG r = PamRunStack(PamBase, handle, PAM_GROUP_AUTH, PAM_OP_SETCRED, flags);
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

    LONG r = PamRunStack(PamBase, handle, PAM_GROUP_ACCOUNT, PAM_OP_ACCT_MGMT, flags);
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

    LONG r = PamRunStack(PamBase, handle, PAM_GROUP_SESSION, PAM_OP_OPEN_SESSION, flags);
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

    LONG r = PamRunStack(PamBase, handle, PAM_GROUP_SESSION, PAM_OP_CLOSE_SESSION, flags);
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
        with PAM_UPDATE_AUTHTOK. Either flag alone runs one pass.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG r = PAM_SUCCESS;
    ULONG passes = flags & (PAM_PRELIM_CHECK | PAM_UPDATE_AUTHTOK);

    if (!passes)
        passes = PAM_PRELIM_CHECK | PAM_UPDATE_AUTHTOK;
    flags &= ~(PAM_PRELIM_CHECK | PAM_UPDATE_AUTHTOK);
    if (passes & PAM_PRELIM_CHECK)
        r = PamRunStack(PamBase, handle, PAM_GROUP_PASSWORD, PAM_OP_CHAUTHTOK, flags | PAM_PRELIM_CHECK);
    if (r == PAM_SUCCESS && (passes & PAM_UPDATE_AUTHTOK))
        r = PamRunStack(PamBase, handle, PAM_GROUP_PASSWORD, PAM_OP_CHAUTHTOK, flags | PAM_UPDATE_AUTHTOK);
    handle->LastStatus = r;
    return r;

    AROS_LIBFUNC_EXIT
}
