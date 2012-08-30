/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <aros/asmcall.h>

/* Smallest possible AROS program */
AROS_ENTRY(__startup ULONG, True,
	   AROS_UFHA(char *, argstr, A0),
	   AROS_UFHA(ULONG, argsize, D0),
	   struct ExecBase *, sBase)
{
    AROS_USERFUNC_INIT

    return (sBase != 0) ? RETURN_OK : RETURN_FAIL;

    AROS_USERFUNC_EXIT
}
