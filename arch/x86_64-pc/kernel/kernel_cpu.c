#include <exec/lists.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_intr.h"
#include "kernel_scheduler.h"

#include "etask.h"

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

    /* 
     * Restore the fpu, mmx, xmm state
     * TODO: Change to the lazy saving of the XMM state!!!!
     */
    if (ctx->Flags & ECF_FPX)
	asm volatile("fxrstor (%0)"::"r"(ctx->FXData));

    /*
     * Leave interrupt and jump to the new task.
     * We will restore CPU state right from this buffer,
     * so no need to copy anything.
     */
    core_LeaveInterrupt(ctx);
}

void cpu_Switch(struct ExceptionContext *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct ExceptionContext *ctx = GetIntETask(task)->iet_Context;

    /*
     * Copy current task's context into the ETask structure. Note that context on stack
     * misses SSE data pointer.
     */
    CopyMemQuick(regs, ctx, sizeof(struct ExceptionContext) - sizeof(struct FPXContext *));

    /*
     * Copy the fpu, mmx, xmm state
     * TODO: Change to the lazy saving of the XMM state!!!!
     */
    asm volatile("fxsave (%0)"::"r"(ctx->FXData));

    /* We have the complete data now */
    ctx->Flags = ECF_SEGMENTS | ECF_FPX;

    /* Set task's tc_SPReg */
    task->tc_SPReg = (APTR)regs->rsp;

    core_Switch();
}
