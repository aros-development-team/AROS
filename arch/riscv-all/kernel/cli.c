/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Disable interrupts, RISC-V version.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <asm/cpu.h>

#include <kernel_base.h>

#include <proto/kernel.h>

/* See rom/kernel/cli.c for documentation */

AROS_LH0I(void, KrnCli,
          struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    csr_clear(RV_CSR_STATUS, RV_STATUS_IE);

    AROS_LIBFUNC_EXIT
}
