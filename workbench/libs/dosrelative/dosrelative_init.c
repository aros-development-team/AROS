/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: dosrelative.library initialisation.
*/

#include <exec/types.h>
#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "dosrelative_intern.h"

#include LC_LIBDEFS_FILE

static int DosRelative_Init(struct DosRelativeBase *DOSRelativeBase)
{
    DOSRelativeBase->drb_DosBase = (struct DosLibrary *)OpenLibrary("dos.library", 50);

    return DOSRelativeBase->drb_DosBase != NULL;
}

static int DosRelative_Expunge(struct DosRelativeBase *DOSRelativeBase)
{
    if (DOSRelativeBase->drb_DosBase)
        CloseLibrary(&DOSRelativeBase->drb_DosBase->dl_lib);

    return TRUE;
}

ADD2INITLIB(DosRelative_Init, 0)
ADD2EXPUNGELIB(DosRelative_Expunge, 0)
