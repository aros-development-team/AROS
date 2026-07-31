/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Report supervisor state, 64bit RISC-V version.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/* See rom/kernel/issuper.c for documentation */

/*
 * Everything runs in S-mode on top of OpenSBI until user-mode task
 * separation exists, so the privilege level can not distinguish task
 * from kernel context. What callers (e.g. exec semaphores) actually
 * need to know is whether we are in trap/interrupt context, where
 * sleeping is impossible - the platform trap handler maintains this.
 */
int __riscv64_trap_depth;

AROS_LH0I(int, KrnIsSuper,
          struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    return __riscv64_trap_depth > 0;

    AROS_LIBFUNC_EXIT
}
