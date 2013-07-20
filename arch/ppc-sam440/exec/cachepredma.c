/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CachePreDMA() - Do what is necessary for DMA.
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/kernel.h>

#include "exec_intern.h"

/* See rom/exec/cachepredma.c for documentation */

#include <proto/exec.h>

AROS_LH3(APTR, CachePreDMA,
    AROS_LHA(APTR,    address, A0),
    AROS_LHA(ULONG *, length,  A1),
    AROS_LHA(ULONG,   flags,  D0),
    struct ExecBase *, SysBase, 127, Exec)
{
    AROS_LIBFUNC_INIT

    void *addr = KrnVirtualToPhysical(address);

    D(bug("[exec] CachePreDMA(%08x, %d, %c) = %08x\n", address, *length, flags & DMA_ReadFromRAM ? 'R':'W', addr));

    /* At PreDMA stage only data caches need to be flushed */
    CacheClearE(address, *length, CACRF_ClearD);

    return addr;

    AROS_LIBFUNC_EXIT
} /* CachePreDMA() */

