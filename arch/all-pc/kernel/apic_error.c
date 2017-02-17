/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <asm/cpu.h>
#include <asm/io.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_globals.h"

#define D(x)

void core_APICErrorHandle(struct ExceptionContext *regs, void *HandlerData, void *HandlerData2)
{
    struct KernelBase *KernelBase = getKernelBase();

    bug("[Exec] %s(0x%p)\n", __func__, KernelBase);

    return;
}
