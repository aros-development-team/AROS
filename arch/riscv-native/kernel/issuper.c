/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include "kernel_riscv.h"

#include <proto/kernel.h>

/* See rom/kernel/issuper.c for documentation */

/*
 * Everything runs in S-mode until user-mode task separation exists, so
 * the privilege level can not distinguish task from kernel context.
 * What callers (e.g. exec semaphores) actually need to know is whether
 * we are in trap/interrupt context, where sleeping is impossible - the
 * platform trap handler maintains the depth (per hart on SMP, where it
 * lives in the TLS).
 */
#if !defined(__AROSEXEC_SMP__)
int __riscv64_trap_depth;
#endif

AROS_LH0I(int, KrnIsSuper,
          struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    return KRN_TRAPDEPTH > 0;

    AROS_LIBFUNC_EXIT
}
