/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: pam.library initialisation
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "pam_intern.h"

static int Pam_Init(struct PamBase *PamBase)
{
    InitSemaphore(&PamBase->ModuleSem);
    NEWLIST(&PamBase->Modules);

    if (!(PamBase->pam_DOSBase = OpenLibrary("dos.library", 36)))
        return FALSE;
    if (!(PamBase->pam_UtilityBase = OpenLibrary("utility.library", 36)))
        return FALSE;
    return TRUE;
}

static int Pam_Expunge(struct PamBase *PamBase)
{
    PamExpungeModules(PamBase);
    if (PamBase->ModuleDir)
        FreeVec(PamBase->ModuleDir);
    if (PamBase->pam_UtilityBase)
        CloseLibrary(PamBase->pam_UtilityBase);
    if (PamBase->pam_DOSBase)
        CloseLibrary(PamBase->pam_DOSBase);
    return TRUE;
}

ADD2INITLIB(Pam_Init, 0);
ADD2EXPUNGELIB(Pam_Expunge, 0);
