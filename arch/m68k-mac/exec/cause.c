/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: cause.c 24162 2006-03-14 15:49:21Z MastaTabs $

    Desc: Plam Version of Cause(); taken from i386native version of Cause().
    Lang: english
*/

#include <exec/execbase.h>
#include <aros/asmcall.h>
#include <exec/interrupts.h>
#include <hardware/custom.h>
#include <hardware/intbits.h>
#include <proto/exec.h>

#include <exec_intern.h>


#include <exec/ptrace.h>

static char softblock;

#if 0
#define get_cs() \
	({  short __value; \
	__asm__ __volatile__ ("mov %%ds,%%ax":"=a"(__value)); \
	(__value & 0x03);	})
#endif

AROS_LH1(void, Cause,
    AROS_LHA(struct Interrupt *, softint, A1),
    struct ExecBase *, SysBase, 30, Exec)
{
	AROS_LIBFUNC_INIT

	UBYTE pri;

	/* Check to ensure that this node is not already in a list. */
	if( softint->is_Node.ln_Type != NT_SOFTINT )
	{
		/* Scale the priority down to a number between 0 and 4 inclusive
		We can use that to index into exec's software interrupt lists. */
		pri = (softint->is_Node.ln_Pri + 0x20) >> 4;

		/* We are accessing an Exec list, protect ourselves. */
		Disable();
		AddTail((struct List *)&SysBase->SoftInts[pri], (struct Node *)softint);
		softint->is_Node.ln_Type = NT_SOFTINT;
		SysBase->SysFlags |= SFF_SoftInt;
		Enable();

		if (!softblock)
		{
			/* If we are in supervisor mode we simply call InterruptServer.
			Cause software interrupt (int 0x80) otherwise */
#if 0
			if (get_cs())
			{
				/* Called from user mode. We can do Cause in normal way */
				__asm__ __volatile__ ("movl $0,%%eax\n\tint $0x80":::"eax","memory");
			}
			else
#endif
			{
				/* Cause() inside supervisor mode. We will call IntServer directly
				no matter it is normal irq or Supervisor() call */
				struct IntVector *iv;
				iv = &SysBase->IntVects[INTB_SOFTINT];

				if (iv->iv_Code)
				{
					  AROS_UFC5(void, iv->iv_Code,
					      AROS_UFCA(ULONG, 0, D1),
					      AROS_UFCA(ULONG, 0, A0),
					      AROS_UFCA(APTR, NULL, A1),
					      AROS_UFCA(APTR, iv->iv_Code, A5),
					      AROS_UFCA(struct ExecBase *, SysBase, A6)
					  );
				}
			}
		}
	}

	AROS_LIBFUNC_EXIT
} /* Cause() */

extern void SaveRegs(struct Task *task, struct pt_regs *regs);
extern void RestoreRegs(struct Task *task, struct pt_regs *regs);

//#define SC_ENABLE(regs)		(regs.eflags |= 0x200)
//#define SC_DISABLE(regs)	(regs.eflags &= ~0x200)


/*asmlinkage*/ void sys_Cause(struct pt_regs * regs)
{
	struct IntVector *iv;

	struct ExecBase * SysBase = 0x04;

	/* Hmm, interrupts are nesting, not a good idea... */
	if(!user_mode(regs)) {
#if NOISY
		kprintf("Illegal Supervisor\n");
#endif
		return;
	}

	iv = &SysBase->IntVects[INTB_SOFTINT];

	if (iv->iv_Code)
	{
		/*  Call it. I call with all these parameters for a reason.

		In my `Amiga ROM Kernel Reference Manual: Libraries and
		Devices' (the 1.3 version), interrupt servers are called
		with the following 5 parameters.

		D1 - Mask of INTENAR and INTREQR
		A0 - 0xDFF000 (base of custom chips)
		A1 - Interrupt Data
		A5 - Interrupt Code vector
		A6 - SysBase

		It is quite possible that some code uses all of these, so
		I must supply them here. Obviously I will dummy some of these
		though.
		*/
		AROS_UFC5(void, iv->iv_Code,
		    AROS_UFCA(ULONG, 0, D1),
		    AROS_UFCA(ULONG, 0, A0),
		    AROS_UFCA(APTR, regs, A1),
		    AROS_UFCA(APTR, iv->iv_Code, A5),
		    AROS_UFCA(struct ExecBase *, SysBase, A6)
		);
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
#if 0
		if (SysBase->IDNestCnt < 0)
			SC_ENABLE(regs);
		else
			SC_DISABLE(regs);
#endif
		/* Ok, the next step is to either drop back to the new task, or
		   give it its Exception() if it wants one... */

		if (SysBase->ThisTask->tc_Flags & TF_EXCEPT)
		{
			Disable();
			Exception();
			Enable();
		}


#if DEBUG_TT
		if (lastTask != SysBase->ThisTask)
		{
			kprintf (stderr, "TT %s\n", SysBase->ThisTask->tc_Node.ln_Name);
			lastTask = SysBase->ThisTask;
		}
#endif
	}

	/* Leave the interrupt. */
}

#undef SysBase

/*
    This is the dispatcher for software interrupts. We go through the
    lists and remove the interrupts before calling them. Because we
    can be interrupted we should not cache the next pointer, but
    retreive it from ExecBase each time.

    Note: All these arguments are passed to the structure, so you must
    at least declare all of them. You do not however have to use any
    of them (although that is recommended).

    This procedure could be more efficient, and it has to be implemented
    in the kernel.
*/

AROS_UFH5(void, SoftIntDispatch,
    AROS_UFHA(ULONG, intReady, D1),
    AROS_UFHA(struct Custom *, custom, A0),
    AROS_UFHA(IPTR, intData, A1),
    AROS_UFHA(IPTR, intCode, A5),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
	AROS_USERFUNC_INIT

	struct Interrupt *intr;
	UBYTE i;

	if( SysBase->SysFlags & SFF_SoftInt )
	{
		/* Disable software interrupts */
		softblock = 1;

		/* Clear the Software interrupt pending flag. */
		SysBase->SysFlags &= ~(SFF_SoftInt);

		for(i=0; i < 4; i++)
		{
			while( (intr = (struct Interrupt *)RemHead((struct List *)&SysBase->SoftInts[i])) )
			{
		  		intr->is_Node.ln_Type = NT_INTERRUPT;

		 		/* Call the software interrupt. */
		  		AROS_UFC3(void, intr->is_Code,
				   AROS_UFCA(APTR, intr->is_Data, A1),
				   AROS_UFCA(APTR, intr->is_Code, A5),
				   AROS_UFCA(struct ExecBase *, SysBase, A6));
			}
		}

		/* We now re-enable software interrupts. */
		softblock = 0;
	}
	AROS_USERFUNC_EXIT
}

