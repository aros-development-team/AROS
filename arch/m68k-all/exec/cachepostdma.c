/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: CachePostDMA() - Do what is necessary for DMA.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

#include <defines/exec_LVO.h>

extern void AROS_SLIB_ENTRY(CachePostDMA_00,Exec,LVOCachePostDMA)(void);
extern void AROS_SLIB_ENTRY(CachePostDMA_30,Exec,LVOCachePostDMA)(void);
extern void AROS_SLIB_ENTRY(CachePostDMA_40,Exec,LVOCachePostDMA)(void);

#include <proto/exec.h>

/* See rom/exec/cachepostdma.c for documentation */

AROS_LH3(void, CachePostDMA,
    AROS_LHA(APTR,    address, A0),
    AROS_LHA(ULONG *, length,  A1),
    AROS_LHA(ULONG,   flags,  D0),
    struct ExecBase *, SysBase, 128, Exec)
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    /* When called the first time, this patches up the
     * Exec syscall table to directly point to the right routine.
     */
    Disable();
    if (SysBase->AttnFlags & AFF_68040) {
        /* 68040 support */
        func = AROS_SLIB_ENTRY(CachePostDMA_40, Exec, LVOCachePostDMA);
    } else if (SysBase->AttnFlags & AFF_68030) {
        /* 68030 support */
        func = AROS_SLIB_ENTRY(CachePostDMA_30, Exec, LVOCachePostDMA);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CachePostDMA_00, Exec, LVOCachePostDMA);
    }

    SetFunction((struct Library *)SysBase, -LVOCachePostDMA * LIB_VECTSIZE, func);
    Enable();

    /* Call 'myself', which is now pointing to the correct routine */
    return CachePostDMA(address, length, flags);

    AROS_LIBFUNC_EXIT
} /* CachePostDMA */

