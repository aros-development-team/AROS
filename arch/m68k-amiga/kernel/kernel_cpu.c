/*
 * M68K Schedule functions
 */

#include <exec/execbase.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <defines/kernel.h>

#include "etask.h"

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_scheduler.h"

#ifndef D
# ifdef DEBUG
#  define D(x) x
# else
#  define D(x)
# endif
#endif

void cpu_Switch(regs_t *regs)
{
    struct Task *task = SysBase->ThisTask;
    struct AROSCPUContext *ctx = GetIntETask(task)->iet_Context;

    /* Actually save the context */
    CopyMem(regs, ctx, sizeof(regs_t));

    /* Update tc_SPReg */
    task->tc_SPReg = (APTR)regs->a[7];

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
    struct AROSCPUContext *ctx;

    for (;;) {
        asm volatile ("move #0x2700, %sr\n");    // Disable CPU interrupts

        task = core_Dispatch();
        if (task != NULL)
            break;
        D(bug("-- IDLE HALT --\n"));
        KrnSti();	// Enable hardware IRQs
        asm volatile ("stop #0x2000\n"); // Wait for an interrupt
    }

    ctx = GetIntETask(task)->iet_Context;
    CopyMem(ctx, regs, sizeof(regs_t));
    regs->a[7] = (IPTR)task->tc_SPReg;

    /* Re-enable interrupts if needed */
    if (SysBase->IDNestCnt < 0)
        KrnSti();
    else
        KrnCli();

    if (task->tc_Flags & TF_EXCEPT) {
    	/* Exec_Exception() will Enable() */
    	Disable();

    	task->tc_SPReg -= sizeof(regs_t);
    	if (task->tc_SPReg <= task->tc_SPLower)
    		Alert(AT_DeadEnd|AN_StackProbe);

    	ctx->a[7] = (IPTR)task->tc_SPReg;
	CopyMem(ctx, task->tc_SPReg, sizeof(regs_t));

	/* Manipulate the current CPU context so Exec_Exception gets
	 * executed after we leave Supervisor mode.
	 */
	regs->a[7] = (IPTR)task->tc_SPReg;
    }
}
