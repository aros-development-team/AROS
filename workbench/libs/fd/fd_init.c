/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>

#include "fd_private.h"
#include LC_LIBDEFS_FILE

static int FD_Init(LIBBASETYPEPTR LIBBASE)
{
    ULONG i;

    InitSemaphore(&LIBBASE->fd_Lock);
    LIBBASE->fd_Table = NULL;
    LIBBASE->fd_Slots = 0;

    for (i = 0; i < FD_OWNER_MAX; i++)
    {
        LIBBASE->fd_OwnerHooks[i] = NULL;
        LIBBASE->fd_OwnerUsers[i] = 0;
    }

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
