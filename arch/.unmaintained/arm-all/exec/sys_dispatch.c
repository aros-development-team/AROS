/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The task Dispatcher for m68k; taken from i386native version of Cause().
    Lang: english
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <exec_intern.h>
#include <exec/ptrace.h>
#include "etask.h"

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

void SaveRegs(struct Task *task, struct pt_regs *regs)
{
	/* Copy registers from struct pt_regs into the user stack */
	struct pt_regs * dest = (struct pt_regs *)task->tc_UnionETask.tc_ETask->et_RegFrame;
	/*
	 * Use this rather than memcpy! It does NOT work otherwise
	 */
#if 1
	dest->r0     = regs->r0;
	dest->r1     = regs->r1;
	dest->r2     = regs->r2;
	dest->r3     = regs->r3;
	dest->r4     = regs->r4;
	dest->r5     = regs->r5;
	dest->r6     = regs->r6;
	dest->r7     = regs->r7;
	dest->r8     = regs->r8;
	dest->r9     = regs->r9;
	dest->r10    = regs->r10;
	dest->r11    = regs->r11;
	dest->r12    = regs->r12;
	dest->sp     = regs->sp;
	dest->lr     = regs->lr;
	dest->lr_svc = regs->lr_svc;
	dest->cpsr   = regs->cpsr;
#else
	ULONG i = 0;
	while (i < sizeof(struct pt_regs)) {
		((UBYTE *)dest)[i] = ((UBYTE *)regs)[i];
		i++;
	}
#endif
	task->tc_SPReg = (APTR)regs->sp;
}

void RestoreRegs(struct Task *task, struct pt_regs *regs)
{
	/* Copy registers from the task's stack into struct pt_regs */
	struct pt_regs * src = (struct pt_regs *task->tc_UnionETask.tc_ETask->et_RegFrame;

	/*
	 * Use this rather than memcpy! It does NOT work otherwise
	 */
#if 1
	regs->r0     = src->r0;
	regs->r1     = src->r1;
	regs->r2     = src->r2;
	regs->r3     = src->r3;
	regs->r4     = src->r4;
	regs->r5     = src->r5;
	regs->r6     = src->r6;
	regs->r7     = src->r7;
	regs->r8     = src->r8;
	regs->r9     = src->r9;
	regs->r10    = src->r10;
	regs->r11    = src->r11;
	regs->r12    = src->r12;
	regs->sp     = src->sp;
	regs->lr     = src->lr;
	regs->lr_svc = src->lr_svc;
	regs->cpsr   = src->cpsr;
#else
	ULONG i = 0;
	while (i < sizeof(struct pt_regs)) {
		((UBYTE *)regs)[i] = ((UBYTE *)src)[i];
		i++;
	}
#endif

	task->tc_SPReg = (APTR)regs->sp;
}

#define SC_ENABLE(regs)	 (regs->cpsr &= 0xffffff7f)
#define SC_DISABLE(regs) (regs->cpsr |= 0x000000C0)

void sys_Dispatch(struct pt_regs * regs, LONG adjust)
{
	struct ExecBase * SysBase = (struct ExecBase *)*(ULONG *)0x04;

	/* Hmm, interrupts are nesting, not a good idea... */
//	if(user_mode(regs)) {
//		return;
//	}
#if 0
	D(bug("In %s!!!\n",__FUNCTION__));
	D(bug("lr_svc=%x, r0=%x, r1=%x, r2=%x, r9=%x, r12=%x, sp=%x, lr=%x, cpsr=%x\n",
	      regs->lr_svc,
	      regs->r0,
	      regs->r1,
	      regs->r2,
	      regs->r9,
	      regs->r12,
	      regs->sp,
	      regs->lr,
	      regs->cpsr));
#endif
	/* Check if a task switch is necessary */
	/* 1. There has to be another task in the ready-list */
	/* 2. The first task in the ready list hast to have the
	      same or higher priority than the currently active task */

	if( SysBase->TaskReady.lh_Head->ln_Succ != NULL /*
	   ((BYTE)SysBase->ThisTask->tc_Node.ln_Pri <=   
	    (BYTE)((struct Task *)SysBase->TaskReady.lh_Head)->tc_Node.ln_Pri ) */
	   )
	{
		/* Check if task switch is possible */
		if( SysBase->TDNestCnt < 0 )
		{
			if( SysBase->ThisTask->tc_State == TS_RUN )
			{
				SysBase->ThisTask->tc_State = TS_READY;
				Reschedule(SysBase->ThisTask);
				SysBase->AttnResched |= 0x8000;
	                }
			else if( SysBase->ThisTask->tc_State == TS_REMOVED )
				SysBase->AttnResched |= 0x8000;
		}
		else
			SysBase->AttnResched |= 0x80;
	} 				                  	     


	/* Has an interrupt told us to dispatch when leaving */
	if (SysBase->AttnResched & 0x8000)
	{
		SysBase->AttnResched &= ~0x8000;

		/* Save registers for this task (if there is one...) */
		if (SysBase->ThisTask && SysBase->ThisTask->tc_State != TS_REMOVED) {
			regs->lr_svc -= adjust; // adjust is 0 or -4
			SaveRegs(SysBase->ThisTask, regs);
		}

		/* Tell exec that we have actually switched tasks... */
		Dispatch ();
//D(bug("DISPATCHER: New task: %s\n",SysBase->ThisTask->tc_Node.ln_Name));

		/* Get the registers of the old task */
		RestoreRegs(SysBase->ThisTask, regs);
		regs->lr_svc += adjust; // adjust is 0 or -4

		/* Make sure that the state of the interrupts is what the task
		   expects.
		*/
		if (SysBase->IDNestCnt < 0)
			SC_ENABLE(regs);
		else
			SC_DISABLE(regs);
#if 0
		D(bug("after: lr_svc=%x, r0=%x, r1=%x, r2=%x, r9=%x, r12=%x, sp=%x, lr=%x, cpsr=%x (adjust=%d)\n",
		      regs->lr_svc,
		      regs->r0,
		      regs->r1,
		      regs->r2,
		      regs->r9,
		      regs->r12,
		      regs->sp,
		      regs->lr,
		      regs->cpsr,
		      adjust));
#endif
		/* Ok, the next step is to either drop back to the new task, or
		   give it its Exception() if it wants one... */

		if (SysBase->ThisTask->tc_Flags & TF_EXCEPT) {
			Disable();
			Exception();
			Enable();
		}


	}

	/* Leave the interrupt. */
}
