/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: example.pam - the smallest external pam.library module.

          auth: succeeds when the user name equals the "user=" argument on
          the configuration line (any password), otherwise PAM_AUTH_ERR.
          Everything else: PAM_IGNORE. Shows the shape of a module; an
          ordinary AROS shared library installed as SYS:Libs/PAM/example.pam.
*/

#include <proto/exec.h>
#include <proto/pam.h>
#include <libraries/pam.h>
#include <libraries/pam_module.h>
#include <string.h>

struct Library *PamBase;

static CONST_STRPTR ArgValue(LONG argc, CONST_STRPTR *argv, CONST_STRPTR key)
{
    LONG i, kl = strlen(key);

    for (i = 0; i < argc; i++)
        if (!strncmp(argv[i], key, kl) && argv[i][kl] == '=')
            return argv[i] + kl + 1;
    return NULL;
}

AROS_LH4(LONG, PAMM_Authenticate,
         AROS_LHA(struct PamHandle *, handle, A0),
         AROS_LHA(ULONG, flags, D0),
         AROS_LHA(LONG, argc, D1),
         AROS_LHA(CONST_STRPTR *, argv, A1),
         struct Library *, PamExampleBase, 5, PamExample)
{
    AROS_LIBFUNC_INIT

    CONST_STRPTR user, want = ArgValue(argc, argv, "user");
    LONG r;

    if (!(PamBase = OpenLibrary(PAMNAME, PAMVERSION)))
        return PAM_SYSTEM_ERR;
    r = PamGetUser(handle, &user, NULL);
    if (r == PAM_SUCCESS)
        r = (want && !strcmp(user, want)) ? PAM_SUCCESS : PAM_AUTH_ERR;
    CloseLibrary(PamBase);
    return r;

    AROS_LIBFUNC_EXIT
}

#define IGNORE_FUNC(name, lvo) \
AROS_LH4(LONG, name, \
         AROS_LHA(struct PamHandle *, handle, A0), \
         AROS_LHA(ULONG, flags, D0), \
         AROS_LHA(LONG, argc, D1), \
         AROS_LHA(CONST_STRPTR *, argv, A1), \
         struct Library *, PamExampleBase, lvo, PamExample) \
{ \
    AROS_LIBFUNC_INIT \
    return PAM_IGNORE; \
    AROS_LIBFUNC_EXIT \
}

IGNORE_FUNC(PAMM_SetCred, 6)
IGNORE_FUNC(PAMM_AcctMgmt, 7)
IGNORE_FUNC(PAMM_OpenSession, 8)
IGNORE_FUNC(PAMM_CloseSession, 9)
IGNORE_FUNC(PAMM_ChAuthTok, 10)
