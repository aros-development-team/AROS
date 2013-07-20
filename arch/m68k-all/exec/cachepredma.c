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

extern void AROS_SLIB_ENTRY(CachePreDMA_00,Exec,127)(void);
extern void AROS_SLIB_ENTRY(CachePreDMA_40,Exec,127)(void);

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
        func = AROS_SLIB_ENTRY(CachePreDMA_40, Exec, 127);
    } else {
        /* Everybody else (68000, 68010) */
        func = AROS_SLIB_ENTRY(CachePreDMA_00, Exec, 127);
    }

    SetFunction((struct Library *)SysBase, -LIB_VECTSIZE * 127, func);
    Enable();

    /* Call 'myself', which is now pointing to the correct routine */
    return CachePreDMA(address, length, flags);

    AROS_LIBFUNC_EXIT
} /* CachePreDMA() */
