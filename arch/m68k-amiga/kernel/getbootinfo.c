/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>

#include <kernel_base.h>

#define DEBUG 0
#include <aros/debug.h>
#include "kernel_debug.h"

#include "exec_intern.h"
#undef KernelBase

#include <proto/kernel.h>

/* See rom/kernel/getbootinfo.c for documentation */

AROS_LH0I(struct TagItem *, KrnGetBootInfo,
    struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    D(bug("[Kernel] KrnGetBootInfo()\n"));
    return PrivExecBase(SysBase)->PlatformData.BootMsg;

    AROS_LIBFUNC_EXIT
}
