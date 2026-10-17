/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library built-in module "security": accounts kept by
          security.library.

          auth:     secVerifyUserA() - the comparison is made by the
                    library's authentication process; this module never
                    sees a hash. Argument "hashed" allows a PAMTOK_ENVOY
                    token (which the library still accepts only with
                    HASHEDLOGIN=1 in Security.config, and only from root).
                    A login ticket is kept for setcred (module data
                    PAM_SECURITY_TICKET_DATA) and given back at PamEnd()
                    if nothing used it.
          setcred:  secLoginA() with the ticket on PAM_AROS_TASK
                    (PAM_ESTABLISH_CRED); secLogoutA() for PAM_DELETE_CRED.
          account:  the user must exist, the account must not be locked
                    (Passwd LOCK) and not have expired: SYS:Security/Expiry
                    holds lines "<user> <yyyy-mm-dd>", the last day the
                    account may be used.
          password: secPasswd() for the caller's own account.
          session:  success; the per-login work (profile, last login,
                    $Home) is still done by secLoginA().
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <proto/pam.h>
#include <resources/log.h>
#include <string.h>

#include "pam_intern.h"

#define DATA_TICKET     PAM_SECURITY_TICKET_DATA
#define DATA_LOGGEDIN   "security.loggedin"

static struct Library *OpenSec(void)
{
    struct Library *base = OpenLibrary(SECURITYNAME, 45);

    if (base && base->lib_Version == 45 && base->lib_Revision < 19)
    {
        CloseLibrary(base);
        base = NULL;
    }
    return base;
}

/*
 * While a challenge token is waited for, the library tells this hook what
 * to show; it goes to the conversation as PAM_AROS_STATUS (repeatedly, so a
 * graphical conversation can poll its Cancel button). The conversation
 * gives up with PAM_CONV_ERR.
 */
static LONG MapVerify(ULONG r);

struct StatusConv
{
    struct PamBase      *PamBase;
    struct PamHandle    *Handle;
};

static CONST_STRPTR StatusText(const struct secKeyStatus *ks)
{
    switch (ks->Type)
    {
    case secKS_INSERT:  return "Insert your security key";
    case secKS_TOUCH:   return "Touch your security key";
    case secKS_WAIT:    return "Waiting for the security key...";
    case secKS_DONE:    return NULL;
    }
    return ks->Text;
}

AROS_UFH3(static IPTR, StatusHookFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct secKeyStatus *, ks, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    struct StatusConv *sc = (struct StatusConv *)hook->h_Data;
    struct PamBase *PamBase = sc->PamBase;
    struct pam_message m;
    const struct pam_message *mp = &m;
    struct pam_response *r = NULL;
    LONG res;

    m.msg_style = PAM_AROS_STATUS;
    m.msg = StatusText(ks);
    res = PamDoConverse(PamBase, sc->Handle, 1, &mp, &r);
    if (r)
        PamFreeResponses(PamBase, 1, r);
    return (res == PAM_SUCCESS) ? TRUE : FALSE;

    AROS_USERFUNC_EXIT
}

#define KEYLIST_MAX     8

/*
 * secVERIFY_KEYNEEDED: the user has a challenge token. The library's
 * client side has the token answer (the user plugs it in and touches it),
 * and the answer completes the verification with the same ticket.
 */
static ULONG AnswerChallenge(struct PamBase *PamBase, struct PamHandle *h, struct Library *secBase, CONST_STRPTR user,
                             struct secLoginTicket *ticket, UBYTE *challenge, struct secKeyInfo *list, ULONG count,
                             struct secUserInfo *info, LONG *pamres)
{
    struct StatusConv sc = { PamBase, h };
    struct Hook hook;
    UBYTE *assertion;
    ULONG size = secKEYASSERTIONMAX, which = 0, r;

    if (count > KEYLIST_MAX)
        count = KEYLIST_MAX;
    if (!(assertion = AllocVec(secKEYASSERTIONMAX, MEMF_ANY | MEMF_CLEAR)))
    {
        *pamres = PAM_BUF_ERR;
        return secVERIFY_FAILED;
    }
    memset(&hook, 0, sizeof(hook));
    hook.h_Entry = (HOOKFUNC)StatusHookFunc;
    hook.h_Data = &sc;
    {
        struct TagItem ctags[] =
        {
            { secT_KCOperation,     secKC_ASSERT        },
            { secT_KeyList,         (IPTR)list          },
            { secT_KeyListSize,     count               },
            { secT_KeyChallenge,    (IPTR)challenge     },
            { secT_KeyAssertion,    (IPTR)assertion     },
            { secT_KeyAssertionSize,(IPTR)&size         },
            { secT_KeyIndex,        (IPTR)&which        },
            { secT_StatusHook,      (IPTR)&hook         },
            { TAG_DONE,             0                   }
        };
        r = secKeyClientA(ctags);
    }
    if (r != secKCERR_OK)
    {
        struct TagItem dtags[] = { { secT_DiscardTicket, (IPTR)ticket }, { TAG_DONE, 0 } };

        secVerifyUserA(dtags);
        FreeVec(assertion);
        *pamres = (r == secKCERR_CANCELLED) ? PAM_CONV_ERR : PAM_AUTH_ERR;
        return secVERIFY_FAILED;
    }
    {
        struct TagItem tags[] =
        {
            { secT_UserID,          (IPTR)user                      },
            { secT_Ticket,          (IPTR)ticket                    },
            { secT_KeyAssertion,    (IPTR)assertion                 },
            { secT_KeyAssertionSize,size                            },
            { secT_KeyPublicID,     (IPTR)list[which < count ? which : 0].PublicID },
            { secT_Service,         (IPTR)h->Service                },
            { secT_UserInfo,        (IPTR)info                      },
            { TAG_DONE,             0                               }
        };
        r = secVerifyUserA(tags);
    }
    FreeVec(assertion);
    *pamres = MapVerify(r);
    return r;
}

/* PamEnd() or replacement: give an unused ticket back */
static void TicketCleanup(struct PamHandle *handle, APTR data, LONG status)
{
    struct Library *secBase;

    if (!data)
        return;
    if ((secBase = OpenSec()))
    {
        struct TagItem tags[] = { { secT_DiscardTicket, (IPTR)data }, { TAG_DONE, 0 } };
        secVerifyUserA(tags);
        CloseLibrary(secBase);
    }
    memset(data, 0, sizeof(struct secLoginTicket));
    FreeVec(data);
}

static LONG MapVerify(ULONG r)
{
    switch (r)
    {
    case secVERIFY_OK:           return PAM_SUCCESS;
    case secVERIFY_LOCKED:       return PAM_MAXTRIES;
    case secVERIFY_BUSY:         return PAM_TRY_AGAIN;
    case secVERIFY_NOTSUPPORTED: return PAM_AUTHINFO_UNAVAIL;
    case secVERIFY_UNAVAILABLE:  return PAM_AUTHINFO_UNAVAIL;
    case secVERIFY_BADARGS:      return PAM_AUTH_ERR;
    default:                     return PAM_AUTH_ERR;
    }
}

static LONG SecAuthenticate(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    CONST_STRPTR user, tok;
    struct secUserInfo *info;
    struct secLoginTicket *ticket;
    ULONG r;
    LONG res;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    /* A local login to an account without a password is not asked for
     * one, as with the built-in login request (not from the network, and
     * not with PAM_DISALLOW_NULL_AUTHTOK: the empty token then fails) */
    if (!h->AuthTok && !h->RHost && !(flags & PAM_DISALLOW_NULL_AUTHTOK) && h->AuthTokFormat == PAMTOK_CLEAR &&
        (secBase = OpenSec()))
    {
        BOOL haspw = TRUE, haskeys = FALSE;
        struct TagItem q[] = { { secT_UserID, (IPTR)user }, { secT_HasPassword, (IPTR)&haspw },
                               { secT_HasKeys, (IPTR)&haskeys }, { TAG_DONE, 0 } };

        /* a security key's code is typed into the password prompt */
        if (secQueryUserA(q) && !haspw && !haskeys)
            PamSetString(PamBase, &h->AuthTok, "", TRUE);
        CloseLibrary(secBase);
    }
    if ((res = PamGetAuthTok(h, PAM_AUTHTOK, &tok, NULL)) != PAM_SUCCESS)
        return res;
    if ((flags & PAM_DISALLOW_NULL_AUTHTOK) && !tok[0])
        return PAM_AUTH_ERR;
    if (h->AuthTokFormat == PAMTOK_ENVOY && !PamArgPresent(argc, argv, "hashed"))
        return PAM_AUTHINFO_UNAVAIL;

    if (!(secBase = OpenSec()))
        return PAM_AUTHINFO_UNAVAIL;
    if (!(ticket = AllocVec(sizeof(struct secLoginTicket), MEMF_PUBLIC | MEMF_CLEAR)))
    {
        CloseLibrary(secBase);
        return PAM_BUF_ERR;
    }
    if (!(info = secAllocUserInfo()))
    {
        FreeVec(ticket);
        CloseLibrary(secBase);
        return PAM_BUF_ERR;
    }
    {
        struct secKeyInfo *list = NULL;
        UBYTE challenge[secKEYCHALLENGESIZE];
        ULONG count = 0;

        /* a local login may have to be completed by a challenge token */
        if (!h->RHost)
            list = AllocVec(KEYLIST_MAX * sizeof(struct secKeyInfo), MEMF_ANY | MEMF_CLEAR);
        {
            struct TagItem tags[] =
            {
                { secT_UserID,      (IPTR)user      },
                { h->AuthTokFormat == PAMTOK_ENVOY ? secT_PasswordHash : secT_Password, (IPTR)tok },
                { secT_HashType,    secHASH_ACRYPT_LC },
                { secT_Service,     (IPTR)h->Service },
                { secT_RemoteHost,  (IPTR)h->RHost  },
                { secT_UserInfo,    (IPTR)info      },
                { secT_Ticket,      (IPTR)ticket    },
                { list ? secT_KeyChallenge : TAG_IGNORE, (IPTR)challenge },
                { list ? secT_KeyList : TAG_IGNORE,      (IPTR)list      },
                { secT_KeyListSize, KEYLIST_MAX     },
                { secT_KeyCount,    (IPTR)&count    },
                { TAG_DONE,         0               }
            };
            r = secVerifyUserA(tags);
        }
        res = MapVerify(r);
        if (r == secVERIFY_KEYNEEDED)
        {
            if (list && count)
                r = AnswerChallenge(PamBase, h, secBase, user, ticket, challenge, list, count, info, &res);
            else
            {
                struct TagItem dtags[] = { { secT_DiscardTicket, (IPTR)ticket }, { TAG_DONE, 0 } };

                secVerifyUserA(dtags);
                res = PAM_AUTHINFO_UNAVAIL;
            }
        }
        if (list)
            FreeVec(list);
        memset(challenge, 0, sizeof(challenge));
    }
    if (res == PAM_SUCCESS)
    {
        /* the stored spelling of the user ID */
        PamSetItem(h, PAM_USER, info->UserID);
        /* replaces (and gives back) a ticket from an earlier pass */
        if (PamSetData(h, DATA_TICKET, ticket, TicketCleanup) != PAM_SUCCESS)
            TicketCleanup(h, ticket, PAM_BUF_ERR);
    }
    else
        FreeVec(ticket);
    secFreeUserInfo(info);
    CloseLibrary(secBase);
    return res;
}

static LONG SecSetCred(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    APTR data;
    LONG res = PAM_SUCCESS;

    if (!(secBase = OpenSec()))
        return PAM_CRED_UNAVAIL;

    if (flags & PAM_DELETE_CRED)
    {
        if (PamGetData(h, DATA_LOGGEDIN, &data) == PAM_SUCCESS && data)
        {
            struct TagItem tags[] = { { secT_Task, (IPTR)h->Task }, { secT_Quiet, TRUE }, { TAG_DONE, 0 } };
            secLogoutA(tags);
            PamSetData(h, DATA_LOGGEDIN, NULL, NULL);
        }
    }
    else
    {
        if (PamGetData(h, DATA_TICKET, &data) != PAM_SUCCESS || !data)
            res = PAM_CRED_UNAVAIL;
        else
        {
            struct TagItem tags[] =
            {
                { secT_Ticket, (IPTR)data   },
                { secT_Task,   (IPTR)h->Task },
                { TAG_DONE,    0            }
            };
            if (secLoginA(tags) == secOWNER_NOBODY)
                res = PAM_CRED_ERR;
            else
                PamSetData(h, DATA_LOGGEDIN, (APTR)1, NULL);
            PamSetData(h, DATA_TICKET, NULL, NULL);     /* one use: gives it back if unused */
        }
    }
    CloseLibrary(secBase);
    return res;
}

/* Days since 1978-01-01 of a yyyy-mm-dd date, -1 if malformed */
static LONG DateDays(CONST_STRPTR d)
{
    static const UWORD cum[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    LONG y, m, day, days;

    if (strlen(d) < 10 || d[4] != '-' || d[7] != '-')
        return -1;
    y = (d[0] - '0') * 1000 + (d[1] - '0') * 100 + (d[2] - '0') * 10 + (d[3] - '0');
    m = (d[5] - '0') * 10 + (d[6] - '0');
    day = (d[8] - '0') * 10 + (d[9] - '0');
    if (y < 1978 || m < 1 || m > 12 || day < 1 || day > 31)
        return -1;
    /* leap days between 1978 and the start of year y */
    days = (y - 1978) * 365 + ((y - 1) / 4 - 1977 / 4) - ((y - 1) / 100 - 1977 / 100) + ((y - 1) / 400 - 1977 / 400);
    days += cum[m - 1] + day - 1;
    if (m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        days++;
    return days;
}

/* Has the account's last day (SYS:Security/Expiry) passed? */
static BOOL Expired(struct PamBase *PamBase, CONST_STRPTR user)
{
    char line[128];
    struct DateStamp ds;
    BPTR fh;
    BOOL expired = FALSE;
    ULONG ulen = strlen(user);

    if (!(fh = Open("SYS:Security/Expiry", MODE_OLDFILE)))
        return FALSE;
    DateStamp(&ds);
    while (FGets(fh, line, sizeof(line) - 1))
    {
        STRPTR p = line;

        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == ';' || *p == '#' || Strnicmp(p, user, ulen) || (p[ulen] != ' ' && p[ulen] != '\t'))
            continue;
        p += ulen;
        while (*p == ' ' || *p == '\t')
            p++;
        {
            LONG last = DateDays(p);
            /* a malformed date counts as expired: fail closed */
            expired = (last < 0) || (ds.ds_Days > last);
        }
        break;
    }
    Close(fh);
    return expired;
}

static LONG SecAcctMgmt(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    CONST_STRPTR user;
    LONG res;
    BOOL found, locked = FALSE;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if (!(secBase = OpenSec()))
        return PAM_AUTHINFO_UNAVAIL;
    if (!secIsConfigured())
    {
        CloseLibrary(secBase);
        return PAM_AUTHINFO_UNAVAIL;
    }
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)user }, { secT_AccountLocked, (IPTR)&locked }, { TAG_DONE, 0 } };
        found = secQueryUserA(tags);
    }
    CloseLibrary(secBase);
    if (!found)
        return PAM_USER_UNKNOWN;
    if (locked)
    {
        PamLogA(PamBase, h, LOGF_Flag_Type_Warn, "account %s is locked", (IPTR []){ (IPTR)user });
        return PAM_PERM_DENIED;
    }
    if (Expired(PamBase, user))
    {
        PamLogA(PamBase, h, LOGF_Flag_Type_Warn, "account %s has expired", (IPTR []){ (IPTR)user });
        return PAM_ACCT_EXPIRED;
    }
    return PAM_SUCCESS;
}

static LONG SecChAuthTok(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    struct Library *secBase;
    CONST_STRPTR user, oldtok, newtok;
    struct secUserInfo *info;
    LONG res;
    BOOL maychange = FALSE, ok;

    if ((res = PamGetUser(h, &user, NULL)) != PAM_SUCCESS)
        return res;
    if (!(secBase = OpenSec()))
        return PAM_AUTHTOK_ERR;

    /* Only the caller's own account: secPasswd() works on the task's owner */
    res = PAM_PERM_DENIED;
    if ((info = secAllocUserInfo()))
    {
        info->uid = (UWORD)(secGetTaskOwner(NULL) >> 16);
        if (secGetUserInfo(info, secKeyType_uid) && !strcmp(info->UserID, user))
            res = PAM_SUCCESS;
        secFreeUserInfo(info);
    }
    if (res == PAM_SUCCESS)
    {
        struct TagItem tags[] = { { secT_UserID, (IPTR)user }, { secT_MayChangePassword, (IPTR)&maychange }, { TAG_DONE, 0 } };
        if (!secQueryUserA(tags) || !maychange)
            res = PAM_PERM_DENIED;
    }
    if (res == PAM_SUCCESS)
    {
        if ((res = PamGetAuthTok(h, PAM_OLDAUTHTOK, &oldtok, "Old password: ")) == PAM_SUCCESS)
        {
            if (flags & PAM_PRELIM_CHECK)
            {
                struct TagItem tags[] = { { secT_UserID, (IPTR)user }, { secT_Password, (IPTR)oldtok }, { secT_Service, (IPTR)h->Service }, { TAG_DONE, 0 } };
                res = MapVerify(secVerifyUserA(tags));
                if (res != PAM_SUCCESS && res != PAM_MAXTRIES)
                    res = PAM_AUTHTOK_RECOVERY_ERR;
            }
            else if ((res = PamGetAuthTok(h, PAM_AUTHTOK, &newtok, "New password: ")) == PAM_SUCCESS)
            {
                ok = secPasswd((STRPTR)oldtok, (STRPTR)newtok);
                res = ok ? PAM_SUCCESS : PAM_AUTHTOK_ERR;
            }
        }
    }
    CloseLibrary(secBase);
    return res;
}

static LONG SecSession(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    return PAM_SUCCESS;
}

const struct PamBuiltin PamBuiltinSecurity =
{
    "security", { SecAuthenticate, SecSetCred, SecAcctMgmt, SecSession, SecSession, SecChAuthTok }
};
