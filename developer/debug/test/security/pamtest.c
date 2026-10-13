/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: PamTest - exercise pam.library from the shell.

    PamTest SERVICE/A,USER,PASSWORD/K,HASH/K,RHOST/K,INTERACTIVE/S,LOGIN/S,ACCOUNT/S,NEWPASSWORD/K,QUIT/S

    Runs PamAuthenticate() (and PamAcctMgmt() with ACCOUNT) for USER with
    PASSWORD, or with HASH as an Envoy-style token. INTERACTIVE lets the
    library ask on the console for what is missing. LOGIN also calls
    PamSetCred() and prints the task's owner before and after. PASSWD=new
    changes the password (old = PASSWORD).
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/pam.h>
#include <libraries/pam.h>
#include <string.h>
#include <stdio.h>

struct Library *PamBase;

static ULONG Owner(void)
{
    struct Library *SecurityBase = OpenLibrary("security.library", 0);
    ULONG owner = 0xDEAD;

    if (SecurityBase)
    {
        owner = AROS_LC1(ULONG, secGetTaskOwner, AROS_LCA(struct Task *, NULL, D0), struct Library *, SecurityBase, 7, Security);
        CloseLibrary(SecurityBase);
    }
    return owner;
}

int main(void)
{
    IPTR args[10] = { 0 };
    struct RDArgs *rda;
    struct PamHandle *h;
    LONG r;
    int rc = RETURN_FAIL;

    if (!(rda = ReadArgs("SERVICE/A,USER,PASSWORD/K,HASH/K,RHOST/K,INTERACTIVE/S,LOGIN/S,ACCOUNT/S,NEWPASSWORD/K,QUIT/S", args, NULL)))
    {
        PrintFault(IoErr(), "PamTest");
        return RETURN_ERROR;
    }
    if (!(PamBase = OpenLibrary("pam.library", 1)))
    {
        printf("PamTest: cannot open pam.library\n");
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    {
        struct TagItem tags[] =
        {
            { PAMT_Interactive,   args[5] ? TRUE : FALSE },
            { PAMT_RemoteHost,    (IPTR)args[4] },
            { args[3] ? PAMT_AuthTok : TAG_IGNORE, (IPTR)args[3] },
            { args[3] ? PAMT_AuthTokFormat : TAG_IGNORE, PAMTOK_ENVOY },
            { args[2] ? PAMT_AuthTok : TAG_IGNORE, (IPTR)args[2] },
            { TAG_DONE, 0 }
        };
        h = PamStartA((CONST_STRPTR)args[0], (CONST_STRPTR)args[1], NULL, tags);
    }
    if (!h)
    {
        printf("PamTest: PamStartA failed\n");
        CloseLibrary(PamBase);
        FreeArgs(rda);
        return RETURN_FAIL;
    }

    printf("owner before: %08lx\n", (unsigned long)Owner());
    r = PamAuthenticate(h, 0);
    {
        CONST_STRPTR user = NULL;
        PamGetItem(h, PAM_USER, (APTR *)&user);
        printf("PamAuthenticate: %ld (%s), user '%s'\n", (long)r, PamStrError(h, r), user ? user : (CONST_STRPTR)"");
    }
    if (r == PAM_SUCCESS)
        rc = RETURN_OK;
    if (r == PAM_SUCCESS && args[7])
    {
        r = PamAcctMgmt(h, 0);
        printf("PamAcctMgmt: %ld (%s)\n", (long)r, PamStrError(h, r));
        if (r != PAM_SUCCESS)
            rc = RETURN_WARN;
    }
    if (r == PAM_SUCCESS && args[6])
    {
        r = PamSetCred(h, PAM_ESTABLISH_CRED);
        printf("PamSetCred: %ld (%s)\n", (long)r, PamStrError(h, r));
        printf("owner after: %08lx\n", (unsigned long)Owner());
        if (r != PAM_SUCCESS)
            rc = RETURN_WARN;
    }
    if (args[8])
    {
        PamSetItem(h, PAM_OLDAUTHTOK, (APTR)args[2]);
        PamSetItem(h, PAM_AUTHTOK, (APTR)args[8]);
        r = PamChAuthTok(h, 0);
        printf("PamChAuthTok: %ld (%s)\n", (long)r, PamStrError(h, r));
        if (r != PAM_SUCCESS)
            rc = RETURN_WARN;
    }
    PamEnd(h, r);
    CloseLibrary(PamBase);
    FreeArgs(rda);
    return rc;
}
