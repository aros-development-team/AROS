/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: CachePreDMA() - Do what is necessary for DMA.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>

#include <defines/exec_LVO.h>

extern void AROS_SLIB_ENTRY(CachePreDMA_00,Exec,LVOCachePreDMA)(void);
extern void AROS_SLIB_ENTRY(CachePreDMA_40,Exec,LVOCachePreDMA)(void);

#include <proto/exec.h>

/* See rom/exec/cachepredma.c for documentation */

AROS_LH3(APTR, CachePreDMA,
    AROS_LHA(APTR,    address, A0),
    AROS_LHA(ULONG *, length,  A1),
    AROS_LHA(ULONG,   flags,  D0),
    struct ExecBase *, SysBase, 127, Exec)
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    /* When called the first time, this patches up the
     * Exec syscall table to directly point to the right routine.
     */
    Disable();
    if (SysBase->AttnFlags & AFF_68040) {
        /* 68040 support */
        func = AROS_SLIB_ENTRY(CachePreDMA_40, Exec, LVOCachePreDMA);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CachePreDMA_00, Exec, LVOCachePreDMA);
    }

    SetFunction((struct Library *)SysBase, -LVOCachePreDMA * LIB_VECTSIZE, func);
    Enable();

    /* Call 'myself', which is now pointing to the correct routine */
    return CachePreDMA(address, length, flags);

    AROS_LIBFUNC_EXIT
} /* CachePreDMA() */
