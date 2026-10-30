/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec_platform.h>       /* IDNESTCOUNT_GET */

#include "kernel_base.h"
#include "kernel_intern.h"

extern int Kernel_13_KrnIsSuper(void);

/* See rom/kernel/unmapglobal.c for documentation. */
AROS_LH2I(int, KrnUnmapGlobal,
    AROS_LHA(void *, virt, A0),
    AROS_LHA(uint32_t, length, D0),
    struct KernelBase *, KernelBase, 17, Kernel)
{
    AROS_LIBFUNC_INIT

    IPTR vaddr = (IPTR)virt & ~(IPTR)(PAGE_SIZE - 1);
    IPTR vend = ((IPTR)virt + (IPTR)length + PAGE_SIZE - 1) & ~(IPTR)(PAGE_SIZE - 1);
    struct pt_pool *pool;
    APTR ssp = NULL;
    int ret;

    if (!length)
        return 0;

    /* Before Forbid()/SuperState(), as in mapglobal.c. */
    if (core_PTPoolPrepare(vaddr, vend, 0, TRUE, &pool))
        return 0;

    if (!Kernel_13_KrnIsSuper()) {
        /* Forbid() covers the window from here to the shootdown; see the
           reasoning in mapglobal.c. */
        Forbid();
        ssp = SuperState();
        if (!ssp) {
            Permit();
            core_PTPoolDispose(pool);
            return 0;
        }
    }

    /* Whole pages; a partly covered 2 MiB mapping is split first. */
    ret = core_UnmapGlobal(vaddr, vend, pool);

    /* Same flush ordering, and the same reasoning, as mapglobal.c. */
    core_FlushOwnTLB();

    if (ssp)
        UserState(ssp);

    core_FlushOtherCPUTLB((ssp == NULL) || (IDNESTCOUNT_GET >= 0), KernelBase);

    if (ssp)
        Permit();

    core_PTPoolDispose(pool);

    /* Non-zero on success, 0 on failure - the same on every port */
    return (ret == 0);

    AROS_LIBFUNC_EXIT
}
