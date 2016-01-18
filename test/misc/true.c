/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <aros/asmcall.h>

/* Smallest possible AROS program */
__startup AROS_PROCH(True, argstr, argsize, sBase)
{
    AROS_PROCFUNC_INIT

    return (sBase != 0) ? RETURN_OK : RETURN_FAIL;

    AROS_PROCFUNC_EXIT
}
