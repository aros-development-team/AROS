/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - conversation: the application's hook, or one of the
          library's own (console, preset), and the module-side helpers
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>

#include "pam_intern.h"

void PamFreeResponses(struct PamBase *PamBase, LONG nmsg, struct pam_response *resp)
{
    LONG i;

    if (!resp)
        return;
    for (i = 0; i < nmsg; i++)
        PamStrFree(PamBase, resp[i].resp, TRUE);
    FreeVec(resp);
}

/* Read one line from a console, echoed or not */
static BOOL ReadLine(struct PamBase *PamBase, BPTR in, BPTR out, STRPTR buf, ULONG size, BOOL echo)
{
    ULONG n = 0;
    char c;
    BOOL ok = TRUE;

    if (!in)
        return FALSE;
    if (!echo)
        SetMode(in, 1);
    for (;;)
    {
        if (Read(in, &c, 1) != 1)
        {
            ok = (n > 0);
            break;
        }
        if (c == '\r' || c == '\n')
            break;
        if (c == 3)                     /* CTRL-C */
        {
            ok = FALSE;
            break;
        }
        if (c == 8 || c == 127)
        {
            if (n)
                n--;
            continue;
        }
        if (n < size - 1 && c >= ' ')
            buf[n++] = c;
    }
    buf[n] = '\0';
    if (!echo)
    {
        SetMode(in, 0);
        if (out)
            FPutC(out, '\n');
    }
    return ok;
}

/* The console conversation: prompts on the handle's Input/Output */
static LONG ConsoleConv(struct PamBase *PamBase, struct PamHandle *h, LONG nmsg, const struct pam_message **msgs, struct pam_response *resp)
{
    char buf[PAM_MAXTOKLEN];
    LONG i;
    BOOL ok;

    for (i = 0; i < nmsg; i++)
    {
        switch (msgs[i]->msg_style)
        {
        case PAM_PROMPT_ECHO_OFF:
        case PAM_PROMPT_ECHO_ON:
            if (h->Output && msgs[i]->msg)
            {
                FPuts(h->Output, msgs[i]->msg);
                Flush(h->Output);
            }
            memset(buf, 0, sizeof(buf));
            ok = ReadLine(PamBase, h->Input, h->Output, buf, sizeof(buf), msgs[i]->msg_style == PAM_PROMPT_ECHO_ON);
            if (!ok)
            {
                memset(buf, 0, sizeof(buf));
                return PAM_CONV_ERR;
            }
            resp[i].resp = PamStrDup(PamBase, buf);
            memset(buf, 0, sizeof(buf));
            if (!resp[i].resp)
                return PAM_BUF_ERR;
            break;
        case PAM_ERROR_MSG:
        case PAM_TEXT_INFO:
            if (h->Output && msgs[i]->msg)
            {
                FPuts(h->Output, msgs[i]->msg);
                FPutC(h->Output, '\n');
                Flush(h->Output);
            }
            break;
        default:
            return PAM_CONV_ERR;
        }
    }
    return PAM_SUCCESS;
}

/* The preset conversation: answers only from what the application set */
static LONG PresetConv(struct PamBase *PamBase, struct PamHandle *h, LONG nmsg, const struct pam_message **msgs, struct pam_response *resp)
{
    LONG i;

    for (i = 0; i < nmsg; i++)
    {
        switch (msgs[i]->msg_style)
        {
        case PAM_PROMPT_ECHO_OFF:
            if (!h->AuthTok)
                return PAM_CONV_ERR;
            if (!(resp[i].resp = PamStrDup(PamBase, h->AuthTok)))
                return PAM_BUF_ERR;
            break;
        case PAM_PROMPT_ECHO_ON:
            if (!h->User)
                return PAM_CONV_ERR;
            if (!(resp[i].resp = PamStrDup(PamBase, h->User)))
                return PAM_BUF_ERR;
            break;
        case PAM_ERROR_MSG:
        case PAM_TEXT_INFO:
            break;
        default:
            return PAM_CONV_ERR;
        }
    }
    return PAM_SUCCESS;
}

LONG PamDoConverse(struct PamBase *PamBase, struct PamHandle *h, LONG nmsg, const struct pam_message **msgs, struct pam_response **resp)
{
    struct pam_response *r;
    LONG res;

    if (!h || nmsg <= 0 || !msgs || !resp)
        return PAM_CONV_ERR;
    *resp = NULL;

    if (h->Conv)
    {
        struct PamConvMsg cm;

        cm.NumMsg = nmsg;
        cm.Messages = msgs;
        cm.Responses = NULL;
        cm.UserData = h->ConvData;
        res = (LONG)CALLHOOKPKT(h->Conv, h, &cm);
        if (res == PAM_SUCCESS)
        {
            if (!cm.Responses)
                return PAM_CONV_ERR;
            *resp = cm.Responses;
        }
        return res;
    }

    if (!(r = AllocVec(nmsg * sizeof(struct pam_response), MEMF_CLEAR)))
        return PAM_BUF_ERR;
    if (h->Interactive)
        res = ConsoleConv(PamBase, h, nmsg, msgs, r);
    else
        res = PresetConv(PamBase, h, nmsg, msgs, r);
    if (res != PAM_SUCCESS)
    {
        PamFreeResponses(PamBase, nmsg, r);
        return res;
    }
    *resp = r;
    return PAM_SUCCESS;
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, PamConverse,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(LONG, nmsg, D0),
        AROS_LHA(const struct pam_message **, msgs, A1),
        AROS_LHA(struct pam_response **, resp, A2),

/*  LOCATION */
        struct PamBase *, PamBase, 18, Pam)

/*  FUNCTION
        Module side: put messages and questions to the person through the
        transaction's conversation. On PAM_SUCCESS *resp is an array of
        nmsg responses; free it with FreeVec() after wiping, or let
        PamGetAuthTok()/PamGetUser() do the work for the usual cases.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return PamDoConverse(PamBase, handle, nmsg, msgs, resp);

    AROS_LIBFUNC_EXIT
}

static LONG AskOne(struct PamBase *PamBase, struct PamHandle *h, LONG style, CONST_STRPTR prompt, STRPTR *answer)
{
    struct pam_message m;
    const struct pam_message *mp = &m;
    struct pam_response *r = NULL;
    LONG res;

    m.msg_style = style;
    m.msg = prompt;
    res = PamDoConverse(PamBase, h, 1, &mp, &r);
    if (res != PAM_SUCCESS)
        return res;
    *answer = r ? r[0].resp : NULL;
    if (r)
        r[0].resp = NULL;
    PamFreeResponses(PamBase, 1, r);
    return *answer ? PAM_SUCCESS : PAM_CONV_ERR;
}

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, PamGetUser,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(CONST_STRPTR *, user, A1),
        AROS_LHA(CONST_STRPTR, prompt, A2),

/*  LOCATION */
        struct PamBase *, PamBase, 16, Pam)

/*  FUNCTION
        Module side: the user name, asking through the conversation if it
        is not set yet (prompt, else PAM_USER_PROMPT, else "login: ").

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;
    STRPTR answer;
    LONG res;

    if (!h || !user)
        return PAM_SYSTEM_ERR;
    if (!h->User || !h->User[0])
    {
        res = AskOne(PamBase, h, PAM_PROMPT_ECHO_ON, prompt ? prompt : (h->UserPrompt ? h->UserPrompt : (CONST_STRPTR)"login: "), &answer);
        if (res != PAM_SUCCESS)
            return res;
        PamStrFree(PamBase, h->User, FALSE);
        h->User = answer;
        if (!h->User[0])
            return PAM_CONV_ERR;
    }
    *user = h->User;
    return PAM_SUCCESS;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, PamGetAuthTok,

/*  SYNOPSIS */
        AROS_LHA(struct PamHandle *, handle, A0),
        AROS_LHA(ULONG, item, D0),
        AROS_LHA(CONST_STRPTR *, tok, A1),
        AROS_LHA(CONST_STRPTR, prompt, A2),

/*  LOCATION */
        struct PamBase *, PamBase, 17, Pam)

/*  FUNCTION
        Module side: the token (PAM_AUTHTOK or PAM_OLDAUTHTOK), asking for
        it unechoed through the conversation if it is not set yet.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PamHandle *h = handle;
    STRPTR *slot, answer;
    LONG res;

    if (!h || !tok)
        return PAM_SYSTEM_ERR;
    if (item == PAM_AUTHTOK)
        slot = &h->AuthTok;
    else if (item == PAM_OLDAUTHTOK)
        slot = &h->OldAuthTok;
    else
        return PAM_BAD_ITEM;

    if (!*slot)
    {
        res = AskOne(PamBase, h, PAM_PROMPT_ECHO_OFF, prompt ? prompt : (CONST_STRPTR)"Password: ", &answer);
        if (res != PAM_SUCCESS)
            return res;
        *slot = answer;
    }
    *tok = *slot;
    return PAM_SUCCESS;

    AROS_LIBFUNC_EXIT
}
