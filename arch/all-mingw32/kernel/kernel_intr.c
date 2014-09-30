/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * Architecture-specific interrupt processing. We reimplement
 * core_ExitInterrupt() because of non-typical idle loop implementation,
 * hence the file name.
 */

#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"
#include "kernel_mingw32.h"
#include "kernel_scheduler.h"
#include "kernel_syscall.h"
#include "kernel_traps.h"

#define D(x) /* This may lock up. See notes in host_intr.c */
#define DSC(x)
#define DTRAP(x)

#define IRET return (SysBase->IDNestCnt < 0) ? INT_ENABLE : INT_DISABLE

/*
 * Leave the interrupt. This function receives the register frame used to leave the supervisor
 * mode. It reschedules the task if it was asked for.
 * This implementation differs from generic one because Windows-hosted AROS has very specific
 * idle loop implementation.
 */
void core_ExitInterrupt(CONTEXT *regs)
{
    D(bug("[Scheduler] core_ExitInterrupt\n"));

    /* Soft interrupt requested? It's high time to do it */
    if (SysBase->SysFlags & SFF_SoftInt)
    {
        D(bug("[Scheduler] Causing SoftInt\n"));
        core_Cause(INTB_SOFTINT, 1L << INTB_SOFTINT);
    }

    /* No tasks active (AROS is in idle state)? If yes, just pick up
       a new ready task (if there is any) */
    if (*KernelIFace.SleepState != SLEEP_MODE_OFF)
    {
        cpu_Dispatch(regs);
        return;
    }

    /* If task switching is disabled, leave immediatelly */
    D(bug("[Scheduler] TDNestCnt is %d\n", SysBase->TDNestCnt));
    if (SysBase->TDNestCnt < 0)
    {
        /* 
         * Do not disturb task if it's not necessary. 
         * Reschedule only if switch pending flag is set. Exit otherwise.
         */
        if (SysBase->AttnResched & ARF_AttnSwitch)
        {
            D(bug("[Scheduler] Rescheduling\n"));
            if (core_Schedule())
	    {
		cpu_Switch(regs);
		cpu_Dispatch(regs);
	    }
        }
    }
}

/* This entry point is called by host-side DLL when an IRQ arrives */
int core_IRQHandler(unsigned char *irqs, CONTEXT *regs)
{
    struct IntrNode *in, *in2;

    /* Run handlers for all active IRQs */
    ForeachNodeSafe(KernelBase->kb_Interrupts, in, in2)
    {
	irqhandler_t h = in->in_Handler;

        if (h && (irqs[in->in_nr]))
            h(in->in_HandlerData, in->in_HandlerData2);
    }

    /* IRQ 0 is exec VBlank timer */
    if (irqs[0])
	core_Cause(INTB_VERTB, 1L << INTB_VERTB);

    /* Reschedule tasks and exit */
    core_ExitInterrupt(regs);
    IRET;
}

/* Trap handler entry point */
int core_TrapHandler(unsigned int num, IPTR *args, CONTEXT *regs)
{
    void (*trapHandler)(unsigned long, struct ExceptionContext *) = NULL;
    struct ExceptionTranslation *ex;
    struct AROSCPUContext *ctx;

    switch (num)
    {
    case AROS_EXCEPTION_SYSCALL:
	/* It's a SysCall exception issued by core_raise() */
	DSC(bug("[KRN] SysCall 0x%04X\n", args[0]));
	switch(args[0])
	{
	case SC_SCHEDULE:
	    if (!core_Schedule())
		break;

	case SC_SWITCH:
	    cpu_Switch(regs);

	case SC_DISPATCH:
	    cpu_Dispatch(regs);
	    break;

	case SC_CAUSE:
	    core_ExitInterrupt(regs);
	    break;
	}
	break;

    case AROS_EXCEPTION_RESUME:
        /* Restore saved context and continue */
	ctx = (struct AROSCPUContext *)args[0];
	RESTOREREGS(regs, ctx);
	**KernelIFace.LastError = ctx->LastError;
	break;

    default:
	/* It's something else, likely a CPU trap */
	bug("[KRN] Exception 0x%08lX, SysBase 0x%p, KernelBase 0x%p\n", num, SysBase, KernelBase);

	/* Find out trap handler for caught task */
	if (SysBase)
	{
            struct Task *t = SysBase->ThisTask;

            if (t)
	    {
		bug("[KRN] %s 0x%p (%s)\n", t->tc_Node.ln_Type == NT_TASK ? "Task":"Process", t, t->tc_Node.ln_Name ? t->tc_Node.ln_Name : "--unknown--");
		trapHandler = t->tc_TrapCode;
		D(bug("[KRN] Task trap handler 0x%p\n", trapHandler));
	    }
	    else
		bug("[KRN] No task\n");

	    DTRAP(bug("[KRN] Exec trap handler 0x%p\n", SysBase->TaskTrapCode));
	    if (!trapHandler)
		trapHandler = SysBase->TaskTrapCode;
	}

    	PRINT_CPUCONTEXT(regs);

	/* Translate Windows exception code to CPU and exec trap numbers */
	for (ex = Traps; ex->ExceptionCode; ex++)
	{
	    if (num == ex->ExceptionCode)
		break;
	}
	DTRAP(bug("[KRN] CPU exception %d, AROS exception %d\n", ex->CPUTrap, ex->AROSTrap));

	/* Convert CPU context to AROS structure */
	struct ExceptionContext tmpContext;
	TRAP_SAVEREGS(regs, tmpContext);

	do
	{
	    if (ex->CPUTrap != -1)
	    {
		if (krnRunExceptionHandlers(KernelBase, ex->CPUTrap, &tmpContext))
		    break;
	    }

	    if (trapHandler && (ex->AmigaTrap != -1))
	    {
		/* Call our trap handler. Note that we may return, this means that the handler has
		   fixed the problem somehow and we may safely continue */
		DTRAP(bug("[KRN] Amiga trap %d\n", ex->AmigaTrap));
		trapHandler(ex->AmigaTrap, &tmpContext);

		break;
	    }

	    /* If we reach here, the trap is unhandled and we request the virtual machine to stop */
	    return INT_HALT;
	} while(0);

	TRAP_RESTOREREGS(regs, tmpContext);
    }

    IRET;
}
