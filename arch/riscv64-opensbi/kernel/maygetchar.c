/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: KrnMayGetChar() - console input over the SBI console.
*/

#include <aros/kernel.h>
#include <aros/libcall.h>

#include <kernel_base.h>

#include "kernel_sbi.h"
#include "kernel_intern.h"

#include <proto/kernel.h>

/* See rom/kernel/maygetchar.c for documentation */

AROS_LH0(int, KrnMayGetChar,
         struct KernelBase *, KernelBase, 26, Kernel)
{
    AROS_LIBFUNC_INIT

    return krnSBIGetC();

    AROS_LIBFUNC_EXIT
}
