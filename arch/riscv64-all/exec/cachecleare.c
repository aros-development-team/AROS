/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: CacheClearE() for 64bit RISC-V.
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*
    See rom/exec/cachecleare.c for the documentation.

    RISC-V keeps the two sides of the cache deliberately out of step: a
    store is visible to loads long before the instruction fetcher will
    see it, and only fence.i brings the fetcher up to date. Code just
    read from disk is data until that happens, so without this a freshly
    loaded program is entered on whatever the fetcher had cached before -
    which is how a legal instruction comes back as an illegal one.

    DMA is coherent on these platforms, so nothing needs writing back by
    hand; the two sides only have to be brought into agreement.

    fence.i speaks for the hart that runs it. When more than one is in
    use the others will have to be told as well.
*/

AROS_LH3(void, CacheClearE,
    AROS_LHA(APTR,  address, A0),
    AROS_LHA(IPTR,  length,  D0),
    AROS_LHA(ULONG, caches,  D1),
    struct ExecBase *, SysBase, 107, Exec)
{
    AROS_LIBFUNC_INIT

    /* Let everything already stored be seen before anything is told to
       go and look at it */
    __asm__ __volatile__ ("fence rw, rw" ::: "memory");

    if (caches & (CACRF_ClearI | CACRF_ClearD | CACRF_InvalidateD))
        __asm__ __volatile__ ("fence.i" ::: "memory");

    AROS_LIBFUNC_EXIT
} /* CacheClearE */
