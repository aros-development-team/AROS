/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: dos64.library initialisation.
*/

#include <exec/types.h>
#include <proto/exec.h>

#include <aros/symbolsets.h>

#include "dos64_intern.h"

#include LC_LIBDEFS_FILE

static int Dos64_Init(struct Dos64Base *DOS64Base)
{
    DOS64Base->d64_DosBase = (struct DosLibrary *)OpenLibrary("dos.library", 50);

    return DOS64Base->d64_DosBase != NULL;
}

static int Dos64_Expunge(struct Dos64Base *DOS64Base)
{
    if (DOS64Base->d64_DosBase)
        CloseLibrary(&DOS64Base->d64_DosBase->dl_lib);

    return TRUE;
}

ADD2INITLIB(Dos64_Init, 0)
ADD2EXPUNGELIB(Dos64_Expunge, 0)
