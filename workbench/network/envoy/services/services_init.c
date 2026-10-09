/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: services.library initialisation. nipc.library is opened by the
          first opener and closed by the last one, so that loading
          services.library alone does not start the network.
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "services_intern.h"

static int Services_Init(struct ServicesBase *ServicesBase)
{
    InitSemaphore(&ServicesBase->sb_Sem);
    InitSemaphore(&ServicesBase->sb_OpenSem);
    NEWLIST(&ServicesBase->sb_Services);
    if (!(ServicesBase->sb_UtilityBase = OpenLibrary("utility.library", 36)))
        return FALSE;
    return TRUE;
}

static int Services_Expunge(struct ServicesBase *ServicesBase)
{
    struct Node *n;

    while ((n = RemHead((struct List *)&ServicesBase->sb_Services)))
        FreeVec(n);
    if (ServicesBase->sb_NIPCBase)
        CloseLibrary(ServicesBase->sb_NIPCBase);
    if (ServicesBase->sb_UtilityBase)
        CloseLibrary(ServicesBase->sb_UtilityBase);
    return TRUE;
}

static int Services_Open(struct ServicesBase *ServicesBase)
{
    BOOL ok = TRUE;

    ObtainSemaphore(&ServicesBase->sb_OpenSem);
    if (!ServicesBase->sb_NIPCBase)
    {
        ServicesBase->sb_NIPCBase = OpenLibrary(NIPCNAME, 50);
        if (!ServicesBase->sb_NIPCBase)
            ok = FALSE;
    }
    ReleaseSemaphore(&ServicesBase->sb_OpenSem);
    return ok;
}

/* runs after lib_OpenCnt has been decremented */
static int Services_Close(struct ServicesBase *ServicesBase)
{
    ObtainSemaphore(&ServicesBase->sb_OpenSem);
    if (ServicesBase->sb_Lib.lib_OpenCnt == 0 && ServicesBase->sb_NIPCBase)
    {
        CloseLibrary(ServicesBase->sb_NIPCBase);
        ServicesBase->sb_NIPCBase = NULL;
    }
    ReleaseSemaphore(&ServicesBase->sb_OpenSem);
    return TRUE;
}

ADD2INITLIB(Services_Init, 0)
ADD2EXPUNGELIB(Services_Expunge, 0)
ADD2OPENLIB(Services_Open, 0)
ADD2CLOSELIB(Services_Close, 0)
