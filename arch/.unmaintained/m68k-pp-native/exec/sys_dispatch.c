/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The task Dispatcher for m68k; taken from i386-native version
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <exec_intern.h>
#include <exec/ptrace.h>
#include "etask.h"


void SaveRegs(struct Task *task, struct pt_regs *regs)
{
	/* Copy registers from struct pt_regs into the user stack */
	struct pt_regs * dest = (struct pt_regs *)GetIntETask(task)->iet_Context;
	/*
	 * Use this rather than memcpy! It does NOT work otherwise
	 */
#if 1
	dest->usp = regs->usp;
	dest->d0  = regs->d0;
	dest->d1  = regs->d1;
	dest->d2  = regs->d2;
	dest->d3  = regs->d3;
	dest->d4  = regs->d4;
	dest->d5  = regs->d5;
	dest->d6  = regs->d6;
	dest->d7  = regs->d7;
	dest->a0  = regs->a0;
	dest->a1  = regs->a1;
	dest->a2  = regs->a2;
	dest->a3  = regs->a3;
	dest->a4  = regs->a4;
	dest->a5  = regs->a5;
	dest->a6  = regs->a6;
	dest->sr  = regs->sr;
	dest->pc  = regs->pc;
#else
	ULONG i = 0;
	while (i < sizeof(struct pt_regs)) {
		((UBYTE *)dest)[i] = ((UBYTE *)regs)[i];
		i++;
	}
#endif
	task->tc_SPReg = (APTR)regs->usp;
}

void RestoreRegs(struct Task *task, struct pt_regs *regs)
{
	/* Copy registers from the task's stack into struct pt_regs */
	struct pt_regs * src = (struct pt_regs *)GetIntETask(task)->iet_Context;

	/*
	 * Use this rather than memcpy! It does NOT work otherwise
	 */
#if 1
	regs->usp = src->usp;
	regs->d0  = src->d0;
	regs->d1  = src->d1;
	regs->d2  = src->d2;
	regs->d3  = src->d3;
	regs->d4  = src->d4;
	regs->d5  = src->d5;
	regs->d6  = src->d6;
	regs->d7  = src->d7;
	regs->a0  = src->a0;
	regs->a1  = src->a1;
	regs->a2  = src->a2;
	regs->a3  = src->a3;
	regs->a4  = src->a4;
	regs->a5  = src->a5;
	regs->a6  = src->a6;
	regs->sr  = src->sr;
	regs->pc  = src->pc;
#else
	ULONG i = 0;
	while (i < sizeof(struct pt_regs)) {
		((UBYTE *)regs)[i] = ((UBYTE *)src)[i];
		i++;
	}
#endif

	task->tc_SPReg = (APTR)regs->usp;
}

#define SC_ENABLE(regs)	 (regs->sr &= 0xf8ff)
#define SC_DISABLE(regs) (regs->sr |= 0x0700)

void sys_Dispatch(struct pt_regs * regs)
{
	struct ExecBase * SysBase = (struct ExecBase *)*(ULONG *)0x04;

	/* Hmm, interrupts are nesting, not a good idea... */
	if(!user_mode(regs)) {
		return;
	}

	/* Check if a task switch is necessary */
	/* 1. There has to be another task in the ready-list */
	/* 2. The first task in the ready list hast to have the
	      same or higher priority than the currently active task */

	if( SysBase->TaskReady.lh_Head->ln_Succ != NULL  /* &&
	   ((BYTE)SysBase->ThisTask->tc_Node.ln_Pri <=   
	    (BYTE)((struct Task *)SysBase->TaskReady.lh_Head)->tc_Node.ln_Pri )*/
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
		if (SysBase->ThisTask && SysBase->ThisTask->tc_State != TS_REMOVED)
			SaveRegs(SysBase->ThisTask, regs);

		/* Tell exec that we have actually switched tasks... */
		Dispatch ();

		/* Get the registers of the old task */
		RestoreRegs(SysBase->ThisTask, regs);
		/* Make sure that the state of the interrupts is what the task
		   expects.
		*/
		if (SysBase->IDNestCnt < 0)
			SC_ENABLE(regs);
		else
			SC_DISABLE(regs);
		/* Ok, the next step is to either drop back to the new task, or
		   give it its Exception() if it wants one... */

		if (SysBase->ThisTask->tc_Flags & TF_EXCEPT)
		{
			Disable();
			Exception();
			Enable();
		}


	}

	/* Leave the interrupt. */
}
