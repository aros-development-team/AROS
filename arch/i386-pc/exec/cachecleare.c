/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() - Clear the caches with extended control.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>

void Exec_Wbinvd();

#include <proto/exec.h>

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR, address, A0),
    AROS_LHA(ULONG, length, D0),
    AROS_LHA(ULONG, caches, D1),
    struct ExecBase *, SysBase, 107, Exec)
/*
    INTERNALS
        Although this function is not needed for BM-DMA on x86 due to
        strong cache coherency (the CPU snoops the address lines and
        invalidates all out-of-date cache), it is needed for some other
        operations. For example, when updating graphics memory address
        translation tables, changes may be invisible to the graphics
        card/chip if not explicitly written back from the cache.

        Drivers performing DMA operations should use
        CachePreDMA()/CachePostDMA() instead, which maximise performance
        on x86 by doing nothing!
*/
{
    AROS_LIBFUNC_INIT

    /* A full flush has been requested */
    if (length == 0xFFFFFFFF)
    {
        Supervisor((ULONG_FUNC)Exec_Wbinvd);
        return;
    }

    /* A partial flush requested */
    if (caches & CACRF_ClearD)
    {
        /* TODO: Detect if CPU supports clflush instruction and use it instead
           of wbinvd to provide more optimized cache flushes */
        Supervisor((ULONG_FUNC)Exec_Wbinvd);
    }

    AROS_LIBFUNC_EXIT
} /* CacheClearE */

