/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <proto/exec.h>

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE


static int Nonvolatile_InitLib(LIBBASETYPE *nvBase)
{
    nvBase->nv_nvdBase = OpenLibrary("nvdisk.library", 0);
    return (nvBase->nv_nvdBase != NULL);
}

static int Nonvolatile_ExpungeLib(LIBBASETYPE *nvBase)
{
    CloseLibrary(nvBase->nv_nvdBase);
    return 1;
}

ADD2INITLIB(Nonvolatile_InitLib, 0)
ADD2EXPUNGELIB(Nonvolatile_ExpungeLib, 0)
