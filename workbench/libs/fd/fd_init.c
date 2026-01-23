/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "fd_private.h"
#include LC_LIBDEFS_FILE

static int FD_Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&LIBBASE->fd_Lock);
    LIBBASE->fd_Table = NULL;
    LIBBASE->fd_Slots = 0;

    return TRUE;
}

static int FD_Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->fd_Table)
        FreeVec(LIBBASE->fd_Table);

    LIBBASE->fd_Table = NULL;
    LIBBASE->fd_Slots = 0;

    return TRUE;
}

ADD2INITLIB(FD_Init, 0)
ADD2EXPUNGELIB(FD_Expunge, 0)
