/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: krnBug() for native RISC-V kernels.
*/

#include <aros/config.h>
#include <asm/cpu.h>

#include <stdarg.h>
#include <stdio.h>

#include <kernel_base.h>
#include <kernel_debug.h>

#if defined(DEBUG_USEATOMIC)
#include "kernel_spinlock.h"

/*
 * As on pc (arch/all-pc/kernel/kernel_debug.h and _bug.c), one call's
 * output is written with interrupts off and a lock held, so harts don't
 * mix their lines. The owner (CPU number + 1) lets a hart that faults
 * while it is printing still print its dump, instead of spinning on its
 * own lock.
 */
static spinlock_t krnBugLock;
static volatile ULONG krnBugOwner;
#endif

int krnBug(const char *format, va_list args, APTR kernelBase)
{
    int retval;
#if defined(DEBUG_USEATOMIC)
    unsigned long status = csr_read(RV_CSR_STATUS);
    ULONG self;
    BOOL nested;

    csr_clear(RV_CSR_STATUS, RV_STATUS_IE);
    /* Only now: with interrupts on, the caller could move to another hart */
    self = krnRVCPUNumber() + 1;
    nested = (krnBugOwner == self);
    if (!nested)
    {
        while (!krnRVSpinTryWrite(&krnBugLock))
            krnRVPause();
        krnBugOwner = self;
    }
#endif

    retval = __vcformat(kernelBase, (int (*)(int, void *))krnPutC,
                        format, args);

#if defined(DEBUG_USEATOMIC)
    if (!nested)
    {
        krnBugOwner = 0;
        krnRVSpinReleaseWrite(&krnBugLock);
    }
    if (status & RV_STATUS_IE)
        csr_set(RV_CSR_STATUS, RV_STATUS_IE);
#endif

    return retval;
}
