/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id: $

    Desc: i386 native CPU supplementals for task scheduler
    Lang: english
*/

#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_intern.h"
#include "kernel_intr.h"
#include "kernel_scheduler.h"

#include "etask.h"

#define D(x)

void cpu_Dispatch(struct ExceptionContext *regs)
{
    struct Task *task;
    struct ExceptionContext *ctx;

    /* 
     * Is the list of ready tasks empty? Well, increment the idle switch cound and halt CPU.
     * It should be extended by some plugin mechanism which would put CPU and whole machine
     * into some more sophisticated sleep states (ACPI?)
     */
    while (!(task = core_Dispatch()))
    {
        /* Sleep almost forever ;) */
        __asm__ __volatile__("sti; hlt; cli");

        if (SysBase->SysFlags & SFF_SoftInt)
            core_Cause(INTB_SOFTINT, 1l << INTB_SOFTINT);
    }

    /* TODO: Handle exception
    if (task->tc_Flags & TF_EXCEPT)
        Exception(); */

    /* Get task's context */
    ctx = GetIntETask(task)->iet_Context;
    D(bug("[Kernel] Dispatch task %s, context 0x%p\n", task->tc_Node.ln_Name, ctx));

    /* Restore GPRs first. CopyMemQuick() may use SSE. */
    CopyMemQuick(ctx, regs, offsetof(struct ExceptionContext, FPData));
    /* Then FPU */
    if (ctx->Flags & ECF_FPX)
    {
	/*
	 * We have SSE state, restore it. 
	 * SSE context includes 8087, so we don't have to care about
	 * it separately after this.
	 */
	asm volatile("fxrstor (%0)"::"r"(ctx->FXData));
    }
    else if (ctx->Flags & ECF_FPU)
    {
	/* No SSE, plain 8087 */
	asm volatile("frstor (%0)"::"r"(ctx->FPData));
    }    
}

void cpu_Switch(struct ExceptionContext *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct ExceptionContext *ctx = GetIntETask(task)->iet_Context;

    D(bug("[Kernel] cpu_Switch(), task %s\n", task->tc_Node.ln_Name));

    /* 
     * Copy the fpu, mmx, xmm state.
     * Do this before CopyMemQuick(), because this function
     * can use SSE itself.
     */
    if (KernelBase->kb_ContextFlags & ECF_FPX)
	asm volatile("fxsave (%0)"::"r"(ctx->FXData));
    if (KernelBase->kb_ContextFlags & ECF_FPU)
	asm volatile("fnsave (%0)"::"r"(ctx->FPData));

    /*
     * Copy current task's context into the ETask structure. Note that context on stack
     * misses SSE data pointer.
     */
    CopyMemQuick(regs, ctx, offsetof(struct ExceptionContext, FPData));
    /* We have the complete data now */
    ctx->Flags = ECF_SEGMENTS | KernelBase->kb_ContextFlags;
    /* Set task's tc_SPReg */
    task->tc_SPReg = (APTR)regs->esp;

    core_Switch();
}
