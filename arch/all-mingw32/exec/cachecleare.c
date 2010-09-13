/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: cachecleare.c 34408 2010-09-11 19:37:21Z sonic $

    Desc: CacheClearE() - Clear the caches with extended control, Windows-hosted implementation
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/libcall.h>

#include "exec_intern.h"

AROS_LH3(void, CacheClearE,
	 AROS_LHA(APTR, address, A0),
	 AROS_LHA(ULONG, length, D0),
	 AROS_LHA(ULONG, caches, D1),
	 struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    /* Windows supports only instruction cache flush */
    if (caches & CACRF_ClearI)
	PD(SysBase).FlushInstructionCache(PD(SysBase).MyProcess, address, length);

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
