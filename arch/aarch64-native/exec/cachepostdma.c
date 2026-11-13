/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: CachePostDMA() - Do what is necessary for DMA (AArch64).
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>

#include "exec_intern.h"

/* See rom/exec/cachepostdma.c for documentation */

#include <proto/exec.h>

AROS_LH3(void, CachePostDMA,
    AROS_LHA(APTR,    address, A0),
    AROS_LHA(ULONG *, length,  A1),
    AROS_LHA(ULONG,   flags,  D0),
    struct ExecBase *, SysBase, 128, Exec)
{
    AROS_LIBFUNC_INIT

    D(bug("[exec] CachePostDMA(%p, %d, %c)\n", address, *length,
          flags & DMA_ReadFromRAM ? 'R' : 'W'));

    /*
     * When the device wrote to memory, the lines covering the buffer
     * may still hold what was there before the transfer; drop them so
     * the reader sees the device's data. Clean-and-invalidate rather
     * than plain invalidate: an unaligned buffer shares its edge lines
     * with neighbours whose dirty data must survive, and a clean line
     * writes back nothing.
     */
    if (!(flags & DMA_ReadFromRAM))
        CacheClearE(address, *length, CACRF_ClearD);

    AROS_LIBFUNC_EXIT
} /* CachePostDMA() */
