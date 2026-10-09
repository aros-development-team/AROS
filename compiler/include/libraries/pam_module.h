#ifndef LIBRARIES_PAM_MODULE_H
#define LIBRARIES_PAM_MODULE_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: the interface a pam.library module implements.

          A module is an ordinary AROS shared library, installed as
          SYS:Libs/PAM/<name>.pam, with these six functions at fixed LVOs.
          Each receives the transaction handle, the flags of the application
          call, and the words that follow the module name on its
          configuration line. A function that does not apply returns
          PAM_IGNORE. Modules run in the context of the application that
          started the transaction.
*/

#include <libraries/pam.h>
#include <aros/libcall.h>

/* LVOs */
#define PAMM_LVO_Authenticate       5
#define PAMM_LVO_SetCred            6
#define PAMM_LVO_AcctMgmt           7
#define PAMM_LVO_OpenSession        8
#define PAMM_LVO_CloseSession       9
#define PAMM_LVO_ChAuthTok          10

/* Calling a module function through its base */
#define PamModuleCall(lvo, base, handle, flags, argc, argv) \
    AROS_LC4(LONG, PamModuleEntry, \
             AROS_LCA(struct PamHandle *, (handle), A0), \
             AROS_LCA(ULONG, (flags), D0), \
             AROS_LCA(LONG, (argc), D1), \
             AROS_LCA(CONST_STRPTR *, (argv), A1), \
             struct Library *, (base), (lvo), PamModule)

#endif /* LIBRARIES_PAM_MODULE_H */
