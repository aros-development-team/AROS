/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library - the built-in modules, and the state of security.library
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/security.h>
#include <string.h>

#include "pam_intern.h"

static LONG AlwaysPermit(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    return PAM_SUCCESS;
}

static LONG AlwaysDeny(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    return PAM_PERM_DENIED;
}

static LONG DenyAuth(struct PamBase *PamBase, struct PamHandle *h, ULONG flags, LONG argc, CONST_STRPTR *argv)
{
    return PAM_AUTH_ERR;
}

const struct PamBuiltin PamBuiltinPermit = { "permit", { AlwaysPermit, AlwaysPermit, AlwaysPermit, AlwaysPermit, AlwaysPermit, AlwaysPermit } };
const struct PamBuiltin PamBuiltinDeny   = { "deny",   { DenyAuth, AlwaysDeny, AlwaysDeny, AlwaysDeny, AlwaysDeny, AlwaysDeny } };

static const struct PamBuiltin * const Builtins[] =
{
    &PamBuiltinPermit, &PamBuiltinDeny, &PamBuiltinSecurity, &PamBuiltinUnixPw, NULL
};

const struct PamBuiltin *PamFindBuiltin(CONST_STRPTR name)
{
    int i;

    for (i = 0; Builtins[i]; i++)
        if (!strcmp(Builtins[i]->Name, name))
            return Builtins[i];
    return NULL;
}

LONG PamSecurityState(struct PamBase *PamBase)
{
    struct Library *secBase;
    LONG state = PAM_STATE_ABSENT;

    if (!FindResident(SECURITYNAME))
        return PAM_STATE_ABSENT;
    state = PAM_STATE_UNCONFIGURED;
    if ((secBase = OpenLibrary(SECURITYNAME, 0)))
    {
        if (secBase->lib_Version > 45 || (secBase->lib_Version == 45 && secBase->lib_Revision >= 13))
        {
            if (secIsConfigured())
                state = PAM_STATE_CONFIGURED;
        }
        CloseLibrary(secBase);
    }
    return state;
}
