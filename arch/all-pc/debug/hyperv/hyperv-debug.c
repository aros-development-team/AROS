/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__
#define __KERNEL_NOEXTERNBASE__

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include LC_LIBDEFS_FILE

#define HYPERVDEBUGEXCEPTION    2

#include "hyperv-cpu.h"
#include "hyperv-tasks.h"

static APTR KernelBase;

/* Main Exception Handler */
int HVExceptionHandler(void *ctx, void *handlerData, void *handlerData2)
{
    kprintf("\n[HyperV:DEBUG] %s()\n", __func__);

    kprintf("\n");
   
    HVDEBUGDumpCPUCtx(ctx);

    kprintf("\n");
    kprintf("[HyperV:DEBUG] %s: SysBase @ 0x%p, KernelBase @ 0x%p\n", __func__, SysBase, KernelBase);
    kprintf("[HyperV:DEBUG] %s: SysBase DispCount = %u\n", __func__, SysBase->DispCount);
    kprintf("\n");

    HVDEBUGDumpTasks(handlerData);

#if (0)
    /* return from the exception */
    Dispatch();
#else
    while (1) asm volatile ("hlt");
#endif
}

static LONG HVDebug_Init(LIBBASETYPE LIBBASE)
{
    KernelBase = OpenResource("kernel.resource");
    APTR ehandle;

    kprintf("[HyperV:DEBUG] %s()\n", __func__);

    ehandle = KrnAddExceptionHandler(HYPERVDEBUGEXCEPTION, HVExceptionHandler, KernelBase, NULL);
    if (ehandle)
    {
        kprintf("[HyperV:DEBUG] %s: ExceptionHandler installed\n", __func__);
    }

    return TRUE;
}


ADD2INITLIB(HVDebug_Init, 0)
