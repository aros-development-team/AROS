/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AuthKey - register, list and remove a user's hardware
          authentication tokens (security.library).
*/

/*****************************************************************************

    NAME
        AuthKey

    SYNOPSIS
        USER/A,ADD/S,ENROL/S,LIST/S,REMOVE/S,LABEL/K,SCHEME/K,ID/K,SECRET/K

    LOCATION
        C:

    FUNCTION
        Manages the hardware authentication tokens ("security keys") of an
        account. A user with a registered token must present it at every
        login besides the password. Two kinds exist: a token that types a
        one-time code, which the user enters directly after the password
        (the login takes the two apart), and a token the login talks to
        over USB, which answers a challenge once touched (FIDO U2F).

        ADD registers a token of a SCHEME that a loaded security.library
        plugin implements, with the ID the token presents in every code
        and the SECRET the scheme needs to check codes (hex digits):

          yubico (yubikey.secplugin): a YubiKey slot programmed for Yubico
          OTP. ID is the modhex public id the slot types first; SECRET is
          the slot's AES-128 key followed by its 6-byte private id, 44 hex
          digits. AROS checks the codes itself, so the slot has to be
          programmed with known values (with Yubico's tools on another
          system).

        ENROL registers a token of the second kind that is plugged in:
        the login talks to it (through fido.device) and it is touched;
        a user is asked for their password after that.
          u2f (fido.secplugin): any FIDO U2F or FIDO2 security key. The
          key makes a key handle and a public key for this system; AROS
          keeps those (ID and secret), the key keeps the private key.

        LIST shows the tokens of USER: label, scheme, id, the counter of
        the last accepted code and the registration date. REMOVE takes
        the token LABEL away; a user whose last token is removed logs in
        with the password alone again.

        root may change any account. Users may list and change their own
        tokens; AuthKey asks for their password first.

    INPUTS
        USER      -- the account.
        ADD       -- register a token: LABEL, SCHEME, ID and SECRET.
        ENROL     -- register a plugged-in challenge token: LABEL, and
                     SCHEME (u2f unless given).
        LIST      -- show the registered tokens.
        REMOVE    -- remove the token LABEL.
        LABEL     -- the token's name, up to 31 characters, no spaces.
        SCHEME    -- the scheme, e.g. yubico or u2f.
        ID        -- the id the token presents with every code.
        SECRET    -- the scheme's secret, hex digits.

    RESULT
        Standard DOS return codes.

    NOTES
        Each scheme needs its plugin loaded: "LOADPLUGIN yubikey" and
        "LOADPLUGIN fido" in SYS:Security/Security.config (AMUInit writes
        them). Tokens are kept in SYS:Security/Keys/<user>, readable by
        root only. KEYTIMEOUT there is how long a login waits for a
        challenge token.

    EXAMPLE
        AuthKey user ADD LABEL=blue SCHEME=yubico ID=ccccccbcgujh SECRET=<44 hex digits>
        AuthKey user LIST
        AuthKey user REMOVE LABEL=blue

    SEE ALSO
        Login, Passwd, Accounts

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>
#include <utility/hooks.h>
#include <libraries/security.h>
#include <string.h>

const TEXT version[] = "$VER: AuthKey 45.1 (17.10.2026)";

#define TEMPLATE "USER/A,ADD/S,ENROL/S,LIST/S,REMOVE/S,LABEL/K,SCHEME/K,ID/K,SECRET/K"
enum { ARG_USER, ARG_ADD, ARG_ENROL, ARG_LIST, ARG_REMOVE, ARG_LABEL, ARG_SCHEME, ARG_ID, ARG_SECRET, ARG_COUNT };

/* What the library says while the token is waited for (ENROL) */
AROS_UFH3(static IPTR, StatusFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct secKeyStatus *, ks, A2),
    AROS_UFHA(APTR, unused, A1))
{
    AROS_USERFUNC_INIT

    static ULONG shown;
    CONST_STRPTR text = NULL;

    switch (ks->Type)
    {
    case secKS_INSERT:  text = "Insert the security key..."; break;
    case secKS_TOUCH:   text = "Touch the security key...";  break;
    case secKS_WAIT:    text = "Waiting for the key...";     break;
    default:            text = ks->Text;                     break;
    }
    if (text && ks->Type != shown)
    {
        shown = ks->Type;
        Printf("%s\n", (IPTR)text);
        Flush(Output());
    }
    return (SetSignal(0, 0) & SIGBREAKF_CTRL_C) ? FALSE : TRUE;

    AROS_USERFUNC_EXIT
}

static CONST_STRPTR TypeName(ULONG type)
{
    switch (type)
    {
    case secKEY_OTP:        return "OTP";
    case secKEY_CHALLENGE:  return "CHAL";
    }
    return "?";
}

struct Library *secBase;

static BOOL ReadPassword(CONST_STRPTR prompt, STRPTR buf, ULONG size)
{
    BPTR in = Input(), out = Output();
    ULONG len = 0;
    BOOL done = FALSE, ok = TRUE;
    LONG c;

    FPuts(out, prompt);
    Flush(out);
    SetMode(in, 1);
    do
    {
        c = FGetC(in);
        switch (c)
        {
        case -1:
        case 3:
            ok = FALSE;
            done = TRUE;
            break;
        case '\b':
        case 127:
            if (len)
                len--;
            break;
        case '\r':
        case '\n':
            done = TRUE;
            break;
        default:
            if ((len < size - 1) && (c & 0x7f) > 31)
                buf[len++] = c;
            break;
        }
    } while (!done);
    buf[len] = '\0';
    SetMode(in, 0);
    FPuts(out, "\n");
    Flush(out);
    return ok;
}

static CONST_STRPTR ErrorText(ULONG err)
{
    switch (err)
    {
    case secKAERR_NOTALLOWED:   return "not allowed (root, or the user with the right password)";
    case secKAERR_NOTFOUND:     return "no such user or token";
    case secKAERR_EXISTS:       return "a token with that label or id is registered already";
    case secKAERR_BADARGS:      return "a field is missing or malformed for the scheme";
    case secKAERR_WRITE:        return "the token file could not be written; nothing was changed";
    case secKAERR_UNAVAILABLE:  return "no multi-user system, or the scheme's plugin is not loaded (LOADPLUGIN)";
    case secKAERR_NOTOKEN:      return "no token answered in time, or the enrolment was given up";
    }
    return "unknown error";
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct secLoginTicket ticket, *ticketp = NULL;
    struct secKeyInfo list[16];
    struct Hook statushook;
    ULONG op, count = 0, res;
    char pwd[128];
    char enrolid[secKEYPUBIDSIZE], enrolsecret[2 * secKEYSECRETMAX + 1];
    BOOL enrol;
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "AuthKey");
        return RETURN_FAIL;
    }
    enrol = args[ARG_ENROL] != 0;
    if ((args[ARG_ADD] ? 1 : 0) + (enrol ? 1 : 0) + (args[ARG_LIST] ? 1 : 0) + (args[ARG_REMOVE] ? 1 : 0) != 1)
    {
        PutStr("AuthKey: give one of ADD, ENROL, LIST or REMOVE\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    op = (args[ARG_ADD] || enrol) ? secKA_ADD : (args[ARG_LIST] ? secKA_LIST : secKA_REMOVE);
    if (op == secKA_ADD && !enrol && (!args[ARG_LABEL] || !args[ARG_SCHEME] || !args[ARG_ID] || !args[ARG_SECRET]))
    {
        PutStr("AuthKey: ADD needs LABEL, SCHEME, ID and SECRET\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (enrol && !args[ARG_LABEL])
    {
        PutStr("AuthKey: ENROL needs LABEL\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (enrol && !args[ARG_SCHEME])
        args[ARG_SCHEME] = (IPTR)"u2f";
    memset(&statushook, 0, sizeof(statushook));
    statushook.h_Entry = (HOOKFUNC)StatusFunc;
    if (op == secKA_REMOVE && !args[ARG_LABEL])
    {
        PutStr("AuthKey: REMOVE needs LABEL\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (!(secBase = OpenLibrary(SECURITYNAME, 45)) || (secBase->lib_Version == 45 && secBase->lib_Revision < 19))
    {
        PutStr("AuthKey: security.library 45.19 is needed\n");
        if (secBase)
            CloseLibrary(secBase);
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    /* ENROL: the token first (its id and public key are what ADD takes), the password after */
    if (enrol)
    {
        struct TagItem et[] =
        {
            { secT_KCOperation, secKC_ENROL             },
            { secT_KeyScheme,   (IPTR)args[ARG_SCHEME]  },
            { secT_KeyPublicID, (IPTR)enrolid           },
            { secT_KeySecret,   (IPTR)enrolsecret       },
            { secT_StatusHook,  (IPTR)&statushook       },
            { TAG_DONE,         0                       }
        };
        res = secKeyClientA(et);
        if (res != secKCERR_OK)
        {
            switch (res)
            {
            case secKCERR_NOTOKEN:      PutStr("AuthKey: no token answered in time\n"); break;
            case secKCERR_CANCELLED:    PutStr("AuthKey: given up\n"); break;
            case secKCERR_UNAVAILABLE:  PutStr("AuthKey: no multi-user system, or the scheme's plugin is not loaded (LOADPLUGIN)\n"); break;
            default:                    PutStr("AuthKey: the token refused or failed\n"); break;
            }
            CloseLibrary(secBase);
            FreeArgs(rda);
            return RETURN_FAIL;
        }
        args[ARG_ID] = (IPTR)enrolid;
        args[ARG_SECRET] = (IPTR)enrolsecret;
    }

    /* not root, changing something: prove the password first */
    if (op != secKA_LIST && (secGetTaskOwner(NULL) >> 16) != secROOT_UID)
    {
        if (!ReadPassword("Password: ", pwd, sizeof(pwd)))
        {
            CloseLibrary(secBase);
            FreeArgs(rda);
            return RETURN_WARN;
        }
        {
            struct TagItem vt[] =
            {
                { secT_UserID,  (IPTR)args[ARG_USER] },
                { secT_Password, (IPTR)pwd           },
                { secT_Service, (IPTR)"authkey"      },
                { secT_Ticket,  (IPTR)&ticket        },
                { TAG_DONE,     0                    }
            };
            res = secVerifyUserA(vt);
            memset(pwd, 0, sizeof(pwd));
            if (res != secVERIFY_OK)
            {
                PutStr("AuthKey: the password is wrong\n");
                CloseLibrary(secBase);
                FreeArgs(rda);
                return RETURN_WARN;
            }
            ticketp = &ticket;
        }
    }

    {
        struct TagItem kt[] =
        {
            { secT_KAOperation, op                                  },
            { secT_UserID,      (IPTR)args[ARG_USER]                },
            { ticketp ? secT_Ticket : TAG_IGNORE, (IPTR)ticketp     },
            { secT_KeyType,     enrol ? secKEY_CHALLENGE : secKEY_OTP },
            { args[ARG_LABEL] ? secT_KeyLabel : TAG_IGNORE, (IPTR)args[ARG_LABEL] },
            { args[ARG_SCHEME] ? secT_KeyScheme : TAG_IGNORE, (IPTR)args[ARG_SCHEME] },
            { args[ARG_ID] ? secT_KeyPublicID : TAG_IGNORE, (IPTR)args[ARG_ID] },
            { args[ARG_SECRET] ? secT_KeySecret : TAG_IGNORE, (IPTR)args[ARG_SECRET] },
            { secT_KeyList,     (IPTR)list                          },
            { secT_KeyListSize, sizeof(list) / sizeof(list[0])      },
            { secT_KeyCount,    (IPTR)&count                        },
            { TAG_DONE,         0                                   }
        };
        res = secKeyAdminA(kt);
    }

    if (res != secKAERR_OK)
        Printf("AuthKey: %s\n", (IPTR)ErrorText(res));
    else if (op == secKA_LIST)
    {
        ULONG i, n = count < sizeof(list) / sizeof(list[0]) ? count : sizeof(list) / sizeof(list[0]);

        if (!count)
            Printf("%s has no hardware tokens\n", (IPTR)args[ARG_USER]);
        else
        {
            Printf("Hardware tokens of %s:\n", (IPTR)args[ARG_USER]);
            Printf("%-20s %-5s %-12s %-34s %-10s %s\n", (IPTR)"Label", (IPTR)"Type", (IPTR)"Scheme", (IPTR)"ID", (IPTR)"Counter", (IPTR)"Registered");
            for (i = 0; i < n; i++)
            {
                char id[35];

                /* a challenge token's id (a key handle) is long: its start tells it apart */
                strncpy(id, list[i].PublicID, sizeof(id) - 1);
                id[sizeof(id) - 1] = '\0';
                if (strlen(list[i].PublicID) > sizeof(id) - 1)
                    strcpy(id + sizeof(id) - 4, "...");
                Printf("%-20s %-5s %-12s %-34s %-10lu %s\n", (IPTR)list[i].Label,
                       (IPTR)TypeName(list[i].Type), (IPTR)list[i].Scheme, (IPTR)id,
                       (IPTR)list[i].Counter, (IPTR)list[i].Registered);
            }
            if (count > n)
                Printf("... and %lu more\n", (IPTR)(count - n));
        }
        rc = RETURN_OK;
    }
    else
    {
        Printf("AuthKey: token '%s' %s for %s\n", (IPTR)args[ARG_LABEL], (IPTR)(op == secKA_ADD ? "registered" : "removed"), (IPTR)args[ARG_USER]);
        if (op == secKA_ADD && enrol)
            PutStr("From now on this account logs in with its password and a touch of the plugged-in token.\n");
        else if (op == secKA_ADD)
            PutStr("From now on this account logs in with its password followed by a code from the token.\n");
        rc = RETURN_OK;
    }

    memset(enrolsecret, 0, sizeof(enrolsecret));
    CloseLibrary(secBase);
    FreeArgs(rda);
    return rc;
}
