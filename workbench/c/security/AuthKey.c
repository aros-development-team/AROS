/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: AuthKey - register, list and remove a user's hardware
          authentication tokens (security.library).
*/

/*****************************************************************************

    NAME
        AuthKey

    SYNOPSIS
        USER/A,ADD/S,LIST/S,REMOVE/S,LABEL/K,SCHEME/K,ID/K,SECRET/K

    LOCATION
        C:

    FUNCTION
        Manages the hardware authentication tokens ("security keys") of an
        account. A user with a registered token must present it at every
        login besides the password: the token types a one-time code, which
        the user enters directly after the password, and the login takes
        the two apart.

        ADD registers a token of a SCHEME that a loaded security.library
        plugin implements, with the ID the token presents in every code
        and the SECRET the scheme needs to check codes (hex digits):

          yubico (yubikey.secplugin): a YubiKey slot programmed for Yubico
          OTP. ID is the modhex public id the slot types first; SECRET is
          the slot's AES-128 key followed by its 6-byte private id, 44 hex
          digits. AROS checks the codes itself, so the slot has to be
          programmed with known values (with Yubico's tools on another
          system).

        LIST shows the tokens of USER: label, scheme, id, the counter of
        the last accepted code and the registration date. REMOVE takes
        the token LABEL away; a user whose last token is removed logs in
        with the password alone again.

        root may change any account. Users may list and change their own
        tokens; AuthKey asks for their password first.

    INPUTS
        USER      -- the account.
        ADD       -- register a token: LABEL, SCHEME, ID and SECRET.
        LIST      -- show the registered tokens.
        REMOVE    -- remove the token LABEL.
        LABEL     -- the token's name, up to 31 characters, no spaces.
        SCHEME    -- the scheme, e.g. yubico.
        ID        -- the id the token presents with every code.
        SECRET    -- the scheme's secret, hex digits.

    RESULT
        Standard DOS return codes.

    NOTES
        Each scheme needs its plugin loaded: "LOADPLUGIN yubikey" in
        SYS:Security/Security.config (AMUInit writes it). Tokens are kept
        in SYS:Security/Keys/<user>, readable by root only.

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
#include <libraries/security.h>
#include <string.h>

const TEXT version[] = "$VER: AuthKey 45.1 (17.10.2026)";

#define TEMPLATE "USER/A,ADD/S,LIST/S,REMOVE/S,LABEL/K,SCHEME/K,ID/K,SECRET/K"
enum { ARG_USER, ARG_ADD, ARG_LIST, ARG_REMOVE, ARG_LABEL, ARG_SCHEME, ARG_ID, ARG_SECRET, ARG_COUNT };

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
    }
    return "unknown error";
}

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    struct secLoginTicket ticket, *ticketp = NULL;
    struct secKeyInfo list[16];
    ULONG op, count = 0, res;
    char pwd[128];
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        PrintFault(IoErr(), "AuthKey");
        return RETURN_FAIL;
    }
    if ((args[ARG_ADD] ? 1 : 0) + (args[ARG_LIST] ? 1 : 0) + (args[ARG_REMOVE] ? 1 : 0) != 1)
    {
        PutStr("AuthKey: give one of ADD, LIST or REMOVE\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    op = args[ARG_ADD] ? secKA_ADD : (args[ARG_LIST] ? secKA_LIST : secKA_REMOVE);
    if (op == secKA_ADD && (!args[ARG_LABEL] || !args[ARG_SCHEME] || !args[ARG_ID] || !args[ARG_SECRET]))
    {
        PutStr("AuthKey: ADD needs LABEL, SCHEME, ID and SECRET\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }
    if (op == secKA_REMOVE && !args[ARG_LABEL])
    {
        PutStr("AuthKey: REMOVE needs LABEL\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    if (!(secBase = OpenLibrary(SECURITYNAME, 45)) || (secBase->lib_Version == 45 && secBase->lib_Revision < 18))
    {
        PutStr("AuthKey: security.library 45.18 is needed\n");
        if (secBase)
            CloseLibrary(secBase);
        FreeArgs(rda);
        return RETURN_FAIL;
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
            { secT_KeyType,     secKEY_OTP                          },
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
                Printf("%-20s %-5s %-12s %-34s %-10lu %s\n", (IPTR)list[i].Label,
                       (IPTR)(list[i].Type == secKEY_OTP ? "OTP" : "?"), (IPTR)list[i].Scheme, (IPTR)list[i].PublicID,
                       (IPTR)list[i].Counter, (IPTR)list[i].Registered);
            if (count > n)
                Printf("... and %lu more\n", (IPTR)(count - n));
        }
        rc = RETURN_OK;
    }
    else
    {
        Printf("AuthKey: token '%s' %s for %s\n", (IPTR)args[ARG_LABEL], (IPTR)(op == secKA_ADD ? "registered" : "removed"), (IPTR)args[ARG_USER]);
        if (op == secKA_ADD)
            PutStr("From now on this account logs in with its password followed by a code from the token.\n");
        rc = RETURN_OK;
    }

    CloseLibrary(secBase);
    FreeArgs(rda);
    return rc;
}
