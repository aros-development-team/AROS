/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Enable interrupts, 64bit RISC-V version.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <asm/cpu.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/* See rom/kernel/sti.c for documentation */

AROS_LH0I(void, KrnSti,
          struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    csr_set(sstatus, SSTATUS_SIE);

    AROS_LIBFUNC_EXIT
}
