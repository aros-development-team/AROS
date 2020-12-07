/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __KERNEL_NOLIBBASE__

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/arossupport.h>

#include <aros/x86_64/cpucontext.h>

#include "etask.h"

#include LC_LIBDEFS_FILE

#define HYPERVDEBUGEXCEPTION    2

static APTR KernelBase;

/* Task Info */
int HVDEBUGDumpCPUCtx(void *ctx)
{
    struct ExceptionContext *regs = (struct ExceptionContext *)ctx;

    kprintf("[HyperV:DEBUG] %s: -   Flags=0x%08X\n", __func__, regs->Flags);
    kprintf("[HyperV:DEBUG] %s: -   stack=%04x:%012x rflags=%016x ip=%04x:%012x ds=0x%04X\n", __func__,
                 regs->ss, regs->rsp, regs->rflags, regs->cs, regs->rip, regs->ds);
    kprintf("[HyperV:DEBUG] %s: -   rax=%016lx rbx=%016lx rcx=%016lx rdx=%016lx\n", __func__, regs->rax, regs->rbx, regs->rcx, regs->rdx);	
    kprintf("[HyperV:DEBUG] %s: -   rsi=%016lx rdi=%016lx rbp=%016lx rsp=%016lx\n", __func__, regs->rsi, regs->rdi, regs->rbp, regs->rsp);
    kprintf("[HyperV:DEBUG] %s: -   r08=%016lx r09=%016lx r10=%016lx r11=%016lx\n", __func__, regs->r8, regs->r9, regs->r10, regs->r11);
    kprintf("[HyperV:DEBUG] %s: -   r12=%016lx r13=%016lx r14=%016lx r15=%016lx\n", __func__, regs->r12, regs->r13, regs->r14, regs->r15);
}
