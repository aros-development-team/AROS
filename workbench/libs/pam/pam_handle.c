/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - transactions, items, module data
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "pam_intern.h"

STRPTR PamStrDup(struct PamBase *PamBase, CONST_STRPTR s)
{
    STRPTR d;
    ULONG len;

    if (!s)
        return NULL;
    len = strlen(s) + 1;
    if ((d = AllocVec(len, MEMF_ANY)))
        CopyMem((APTR)s, d, len);
    return d;
}

void PamStrFree(struct PamBase *PamBase, STRPTR s, BOOL wipe)
{
    if (!s)
        return;
    if (wipe)
        memset(s, 0, strlen(s));
    FreeVec(s);
}

BOOL PamSetString(struct PamBase *PamBase, STRPTR *slot, CONST_STRPTR s, BOOL wipe)
{
    STRPTR d = NULL;

    if (s && !(d = PamStrDup(PamBase, s)))
        return FALSE;
    PamStrFree(PamBase, *slot, wipe);
    *slot = d;
    return TRUE;
}

BOOL PamArgPresent(LONG argc, CONST_STRPTR *argv, CONST_STRPTR word)
{
    LONG i;

    for (i = 0; i < argc; i++)
        if (!strcmp(argv[i], word))
            return TRUE;
    return FALSE;
}

/* "key=value" -> value */
CONST_STRPTR PamArgValue(LONG argc, CONST_STRPTR *argv, CONST_STRPTR key)
{
    LONG i, kl = strlen(key);

    for (i = 0; i < argc; i++)
        if (!strncmp(argv[i], key, kl) && argv[i][kl] == '=')
            return argv[i] + kl + 1;
    return NULL;
}

static void FreeData(struct PamBase *PamBase, struct PamHandle *h, LONG status)
{
    struct PamData *d;

    while ((d = (struct PamData *)RemHead((struct List *)&h->Data)))
    {
        if (d->Cleanup)
            d->Cleanup(h, d->Data, status);
        FreeVec(d->Name);
        FreeVec(d);
    }
}

/*****************************************************************************

    NAME */
        AROS_LH4(struct PamHandle *, PamStartA,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, service, A0),
        AROS_LHA(CONST_STRPTR, user, A1),
        AROS_LHA(struct Hook *, conv, A2),
        AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
        struct PamBase *, PamBase, 5, Pam)

/*  FUNCTION
        Begin an authentication transaction for a service. The service name
        selects the file SYS:Security/PAM/<service> that lists the modules
        to run; without one, "other" is used, and without that a built-in
        default that depends on whether security.library is present and
        configured.

    INPUTS
        service - name of the service ("login", "envoy", "sshd", ...).
        user    - the user name, or NULL to let the modules ask.
        conv    - conversation hook, or NULL for one of the library's own
                  (see <libraries/pam.h>).
        tags    - PAMT_#? tags.

    RESULT
        The handle, or NULL.

    SEE ALSO
        PamEnd(), PamAuthenticate(), PamSetItem()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h;
    struct Process *me = (struct Process *)FindTask(NULL);
    BOOL isproc = (me->pr_Task.tc_Node.ln_Type == NT_PROCESS);
    int i;

    if (!service || !service[0])
        return NULL;
    if (!(h = AllocVec(sizeof(struct PamHandle), MEMF_CLEAR)))
        return NULL;

    h->Base = PamBase;
    NEWLIST(&h->Data);
    for (i = 0; i < PAM_NUM_GROUPS; i++)
        NEWLIST(&h->Stack[i]);
    h->Conv = conv;
    h->Task = &me->pr_Task;
    h->Input = isproc ? Input() : BNULL;
    h->Output = isproc ? Output() : BNULL;

    if (!PamSetString(PamBase, &h->Service, service, FALSE) ||
        !PamSetString(PamBase, &h->User, user, FALSE))
    {
        FreeVec(h);
        return NULL;
    }

    if (tags)
    {
        struct TagItem *tag, *tstate = tags;

        while ((tag = NextTagItem(&tstate)))
        {
            switch (tag->ti_Tag)
            {
            case PAMT_Interactive:   h->Interactive = tag->ti_Data ? TRUE : FALSE;      break;
            case PAMT_Input:         h->Input = (BPTR)tag->ti_Data;                     break;
            case PAMT_Output:        h->Output = (BPTR)tag->ti_Data;                    break;
            case PAMT_PubScreen:     PamSetString(PamBase, &h->PubScreen, (STRPTR)tag->ti_Data, FALSE); break;
            case PAMT_RemoteHost:    PamSetString(PamBase, &h->RHost, (STRPTR)tag->ti_Data, FALSE);     break;
            case PAMT_RemoteUser:    PamSetString(PamBase, &h->RUser, (STRPTR)tag->ti_Data, FALSE);     break;
            case PAMT_AuthTok:       PamSetString(PamBase, &h->AuthTok, (STRPTR)tag->ti_Data, TRUE);    break;
            case PAMT_AuthTokFormat: h->AuthTokFormat = tag->ti_Data;                                  break;
            case PAMT_Task:          if (tag->ti_Data) h->Task = (struct Task *)tag->ti_Data;          break;
            case PAMT_ConvData:      h->ConvData = (APTR)tag->ti_Data;                                 break;
            }
        }
    }
    return h;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamEnd,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(LONG, status, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 6, Pam)

/*  FUNCTION
        Finish a transaction. Module data is released (with its cleanup
        functions, which receive status) and the tokens are wiped.

    RESULT
        PAM_SUCCESS.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;

    if (!h)
        return PAM_SYSTEM_ERR;

    FreeData(PamBase, h, status);
    PamFreeStack(PamBase, h);
    PamStrFree(PamBase, h->Service, FALSE);
    PamStrFree(PamBase, h->User, FALSE);
    PamStrFree(PamBase, h->UserPrompt, FALSE);
    PamStrFree(PamBase, h->RHost, FALSE);
    PamStrFree(PamBase, h->RUser, FALSE);
    PamStrFree(PamBase, h->Tty, FALSE);
    PamStrFree(PamBase, h->AuthTokType, FALSE);
    PamStrFree(PamBase, h->AuthTok, TRUE);
    PamStrFree(PamBase, h->OldAuthTok, TRUE);
    PamStrFree(PamBase, h->PubScreen, FALSE);
    PamStrFree(PamBase, h->ErrorText, FALSE);
    FreeVec(h);
    return PAM_SUCCESS;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, PamSetItem,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, item, D0),
        AROS_LHA(APTR, value, A1),

/*  LOCATION */
        struct PamBase *, PamBase, 7, Pam)

/*  FUNCTION
        Set an item of the transaction. String items are copied.

    RESULT
        PAM_SUCCESS, PAM_BAD_ITEM, PAM_BUF_ERR.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;
    BOOL ok = TRUE;

    if (!h)
        return PAM_SYSTEM_ERR;

    switch (item)
    {
    case PAM_SERVICE:       ok = PamSetString(PamBase, &h->Service, value, FALSE); PamFreeStack(PamBase, h); break;
    case PAM_USER:          ok = PamSetString(PamBase, &h->User, value, FALSE);         break;
    case PAM_USER_PROMPT:   ok = PamSetString(PamBase, &h->UserPrompt, value, FALSE);   break;
    case PAM_TTY:           ok = PamSetString(PamBase, &h->Tty, value, FALSE);          break;
    case PAM_RHOST:         ok = PamSetString(PamBase, &h->RHost, value, FALSE);        break;
    case PAM_RUSER:         ok = PamSetString(PamBase, &h->RUser, value, FALSE);        break;
    case PAM_AUTHTOK_TYPE:  ok = PamSetString(PamBase, &h->AuthTokType, value, FALSE);  break;
    case PAM_AUTHTOK:       ok = PamSetString(PamBase, &h->AuthTok, value, TRUE);       break;
    case PAM_OLDAUTHTOK:    ok = PamSetString(PamBase, &h->OldAuthTok, value, TRUE);    break;
    case PAM_CONV:          h->Conv = (struct Hook *)value;                             break;
    case PAM_AROS_INPUT:    h->Input = (BPTR)value;                                     break;
    case PAM_AROS_OUTPUT:   h->Output = (BPTR)value;                                    break;
    case PAM_AROS_PUBSCREEN: ok = PamSetString(PamBase, &h->PubScreen, value, FALSE);   break;
    case PAM_AROS_TASK:     h->Task = value ? (struct Task *)value : FindTask(NULL);    break;
    case PAM_AROS_AUTHTOK_FORMAT: h->AuthTokFormat = (IPTR)value;                       break;
    default:
        return PAM_BAD_ITEM;
    }
    return ok ? PAM_SUCCESS : PAM_BUF_ERR;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, PamGetItem,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, item, D0),
        AROS_LHA(APTR *, value, A1),

/*  LOCATION */
        struct PamBase *, PamBase, 8, Pam)

/*  FUNCTION
        Read an item. The pointer returned belongs to the transaction.

    RESULT
        PAM_SUCCESS, PAM_BAD_ITEM.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;

    if (!h || !value)
        return PAM_SYSTEM_ERR;

    switch (item)
    {
    case PAM_SERVICE:       *value = h->Service;        break;
    case PAM_USER:          *value = h->User;           break;
    case PAM_USER_PROMPT:   *value = h->UserPrompt;     break;
    case PAM_TTY:           *value = h->Tty;            break;
    case PAM_RHOST:         *value = h->RHost;          break;
    case PAM_RUSER:         *value = h->RUser;          break;
    case PAM_AUTHTOK_TYPE:  *value = h->AuthTokType;    break;
    case PAM_AUTHTOK:       *value = h->AuthTok;        break;
    case PAM_OLDAUTHTOK:    *value = h->OldAuthTok;     break;
    case PAM_CONV:          *value = h->Conv;           break;
    case PAM_AROS_INPUT:    *value = (APTR)h->Input;    break;
    case PAM_AROS_OUTPUT:   *value = (APTR)h->Output;   break;
    case PAM_AROS_PUBSCREEN: *value = h->PubScreen;     break;
    case PAM_AROS_TASK:     *value = h->Task;           break;
    case PAM_AROS_AUTHTOK_FORMAT: *value = (APTR)h->AuthTokFormat; break;
    default:
        *value = NULL;
        return PAM_BAD_ITEM;
    }
    return PAM_SUCCESS;

    AROS_LIBFUNC_EXIT
}

static const char * const ErrorTexts[] =
{
    "Success", "Failed to load module", "Module entry point missing", "Error in service configuration",
    "System error", "Memory allocation error", "Permission denied", "Authentication failure",
    "Insufficient credentials to access authentication data", "Authentication service cannot retrieve authentication info",
    "User not known to the underlying authentication module", "Have exhausted maximum number of retries for service",
    "Authentication token is no longer valid; new one required", "User account has expired",
    "Cannot make/remove an entry for the specified session", "Authentication service cannot retrieve user credentials",
    "User credentials expired", "Failure setting user credentials", "No module specific data is present",
    "Conversation error", "Authentication token manipulation error", "Authentication information cannot be recovered",
    "Authentication token lock busy", "Authentication token aging disabled", "Failed preliminary check by password service",
    "The return value should be ignored by PAM dispatch", "Critical error - immediate abort", "Authentication token expired",
    "Module is unknown", "Bad item passed to PamSetItem()/PamGetItem()", "Conversation is waiting for event",
    "Application needs to call libpam again"
};

/*****************************************************************************

    NAME */
        AROS_LH2(CONST_STRPTR, PamStrError,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(LONG, code, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 9, Pam)

/*  FUNCTION
        Text for a result code. handle may be NULL.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (code < 0 || code >= (LONG)(sizeof(ErrorTexts) / sizeof(ErrorTexts[0])))
        return "Unknown PAM error";
    return ErrorTexts[code];

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, PamSetData,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(CONST_STRPTR, name, A1),
        AROS_LHA(APTR, data, A2),
        AROS_LHA(PamDataCleanup, cleanup, A3),

/*  LOCATION */
        struct PamBase *, PamBase, 19, Pam)

/*  FUNCTION
        Module side: attach private data to the transaction under a name.
        Replacing data calls the old cleanup with PAM_SUCCESS; PamEnd()
        calls it with the final status.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;
    struct PamData *d;

    if (!h || !name)
        return PAM_SYSTEM_ERR;

    ForeachNode(&h->Data, d)
    {
        if (!strcmp(d->Name, name))
        {
            if (d->Cleanup)
                d->Cleanup(h, d->Data, PAM_SUCCESS);
            d->Data = data;
            d->Cleanup = cleanup;
            return PAM_SUCCESS;
        }
    }
    if (!(d = AllocVec(sizeof(struct PamData), MEMF_CLEAR)))
        return PAM_BUF_ERR;
    if (!(d->Name = PamStrDup(PamBase, name)))
    {
        FreeVec(d);
        return PAM_BUF_ERR;
    }
    d->Data = data;
    d->Cleanup = cleanup;
    AddTail((struct List *)&h->Data, (struct Node *)d);
    return PAM_SUCCESS;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, PamGetData,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(CONST_STRPTR, name, A1),
        AROS_LHA(APTR *, data, A2),

/*  LOCATION */
        struct PamBase *, PamBase, 20, Pam)

/*  FUNCTION
        Module side: retrieve data stored with PamSetData().

    RESULT
        PAM_SUCCESS or PAM_NO_MODULE_DATA.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;
    struct PamData *d;

    if (!h || !name || !data)
        return PAM_SYSTEM_ERR;
    ForeachNode(&h->Data, d)
    {
        if (!strcmp(d->Name, name))
        {
            *data = d->Data;
            return PAM_SUCCESS;
        }
    }
    *data = NULL;
    return PAM_NO_MODULE_DATA;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, PamFailDelay,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, usec, D0),

/*  LOCATION */
        struct PamBase *, PamBase, 21, Pam)

/*  FUNCTION
        Ask for a delay before PamAuthenticate() returns a failure. The
        largest request wins. The delay is applied only to interactive
        transactions; a network service gets its refusal at once.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;

    if (!h)
        return PAM_SYSTEM_ERR;
    if (usec > h->FailDelay)
        h->FailDelay = usec;
    return PAM_SUCCESS;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(void, PamLog,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(LONG, level, D0),
        AROS_LHA(CONST_STRPTR, text, A1),

/*  LOCATION */
        struct PamBase *, PamBase, 22, Pam)

/*  FUNCTION
        Module side: record a message. Goes to the debug log; a file or
        syslog destination may be added later.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;

    bug(DEBUG_NAME_STR " %s/%s: [%ld] %s\n", h ? h->Service : (STRPTR)"?", (h && h->User) ? h->User : (STRPTR)"?", (long)level, text ? text : (CONST_STRPTR)"");

    AROS_LIBFUNC_EXIT
}
