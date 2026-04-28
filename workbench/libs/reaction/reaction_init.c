/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction library initialization
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <proto/exec.h>

#include "reaction_intern.h"

#include <aros/symbolsets.h>
#include LC_LIBDEFS_FILE

static int Reaction_Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&LIBBASE->aui_Lock);

    if ((LIBBASE->aui_IntuitionBase = OpenLibrary("intuition.library", 39)) != NULL)
    {
        if ((LIBBASE->aui_UtilityBase = OpenLibrary("utility.library", 39)) != NULL)
        {
            return TRUE;
        }
        CloseLibrary((struct Library *)LIBBASE->aui_IntuitionBase);
    }

    return FALSE;
}

static int Reaction_Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->aui_UtilityBase)
        CloseLibrary((struct Library *)LIBBASE->aui_UtilityBase);
    if (LIBBASE->aui_IntuitionBase)
        CloseLibrary((struct Library *)LIBBASE->aui_IntuitionBase);

    return TRUE;
}

ADD2INITLIB(Reaction_Init, 0);
ADD2EXPUNGELIB(Reaction_Expunge, 0);
