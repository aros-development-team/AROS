/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Passwd - change your password (security.library)
*/

/*****************************************************************************

    NAME
        Passwd

    SYNOPSIS
        OLD/K,NEW/K

    LOCATION
        C:

    FUNCTION
        Changes the password of the user owning the current shell. The old
        and new passwords are asked for on the console unless given.

    INPUTS
        OLD -- the current password.
        NEW -- the new password.

    RESULT
        RETURN_WARN if the password could not be changed.

    SEE ALSO
        Login

******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/security.h>

#include <dos/dos.h>
#include <libraries/security.h>

#include <string.h>

const TEXT version[] = "$VER: Passwd 45.1 (28.08.2026)";

#define TEMPLATE "OLD/K,NEW/K"

enum { ARG_OLD, ARG_NEW, ARG_COUNT };

struct Library *secBase;

/* Read a password without echo */
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

int main(void)
{
    IPTR args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    int rc = RETURN_FAIL;
    char oldpwd[secPASSWORDSIZE], newpwd[secPASSWORDSIZE], retype[secPASSWORDSIZE];
    STRPTR o, n;

    if (!(secBase = OpenLibrary(SECURITYNAME, 0)))
    {
        PutStr("Passwd: security.library is not available - single user system\n");
        return RETURN_FAIL;
    }

    if ((rda = ReadArgs(TEMPLATE, args, NULL)))
    {
        rc = RETURN_WARN;
        if (secGetTaskOwner(NULL) == secOWNER_NOBODY)
            PutStr("You are not logged in\n");
        else
        {
            o = (STRPTR)args[ARG_OLD];
            n = (STRPTR)args[ARG_NEW];
            if (!o)
            {
                if (!ReadPassword("Old password        : ", oldpwd, sizeof(oldpwd)))
                    goto done;
                o = oldpwd;
            }
            if (!n)
            {
                if (!ReadPassword("New password        : ", newpwd, sizeof(newpwd)))
                    goto done;
                if (!ReadPassword("Retype new password : ", retype, sizeof(retype)))
                    goto done;
                if (strcmp(newpwd, retype))
                {
                    PutStr("Error retyping new password\n");
                    goto done;
                }
                n = newpwd;
            }
            if (secPasswd(o, n))
            {
                PutStr("Password successfully changed\n");
                rc = RETURN_OK;
            }
            else
                PutStr("Changing password failed\n");
        }
done:
        memset(oldpwd, 0, sizeof(oldpwd));
        memset(newpwd, 0, sizeof(newpwd));
        memset(retype, 0, sizeof(retype));
        FreeArgs(rda);
    }
    else
        PrintFault(IoErr(), "Passwd");

    CloseLibrary(secBase);
    return rc;
}
