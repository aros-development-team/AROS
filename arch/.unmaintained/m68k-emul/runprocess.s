/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Run a process ( invoked by dos/Runcommand() )
    Lang: english

 LONG RunProcess ( struct Process         * proc,
		    struct StackSwapStruct * sss,
		    STRPTR		     argptr,
		    ULONG		     argsize,
		    LONG_FUNC		     entry,
		    struct DosLibrary	   * DOSBase )
*/

	#include "machine.i"

/*
    This is how the stack looks when this function is called:

    20  DOSBase(4)
    16  entry(4)
    12  argsize(4)
     8  argptr(4)
     4  sss(4)
     0  proc(4)
        Return Address(4)


/*	.set FirstArg,	4+(2*4)	/ * Return-address + registers * /
	.set proc,	FirstArg
	.set sss,	proc+4
	.set argptr,	sss+4
	.set argsize,	argptr+4
	.set entry,	argsize+4 */

#if !UseRegisterArgs
#define FirstArg	4+(2*4) /* Return-Address + 2 Registers */
#else
#define FirstArg	4+(4*4)	/* Return-Address + 4 Registers */
#endif
#define proc		FirstArg+0
#define sss		FirstArg+4
#define argptr		FirstArg+8
#define argsize		FirstArg+12
#define entry		FirstArg+16
#define DOSBase         FirstArg+20

	.text
	.balign 16
	.globl	AROS_SLIB_ENTRY(RunProcess,Dos)
	.type	AROS_SLIB_ENTRY(RunProcess,Dos),@function
AROS_SLIB_ENTRY(RunProcess,Dos):
#if !UseRegisterArgs
	movem.l	%a5-%a6,-(%sp)		/* Save some registers */
#else
	movem.l	%d2/%a2/%a5-%a6,-(%sp)	/* Save some registers */
#endif

	move.l	sss(%sp),%a0		/* Fetch the arguments off the stack */
	move.l	entry(%sp),%a5		/* "     "   "         "   "   " */

	/* Move upper bounds of the new stack into a1 */
	move.l	stk_Upper(%a0),%a1

#if !UseRegisterArgs
	/*
	    Push arguments for entry onto the stack of the new process.
	    This new stack looks like this when the new process is called:

		sss
		SysBase
		argsize
		argptr
	*/
	move.l	argptr(%sp),-16(%a1)
	move.l	argsize(%sp),-12(%a1)
	
	move.l	%a0,-4(%a1)		/* Push sss onto the new stack */
#else
	/*
	    We need to save both argptr and argsize from the stack.
	*/
	move.l	argptr(%sp),%a2
	move.l	argsize(%sp),%d2

	move.l	%a0,-(%a1)		/* Push sss onto the new stack */
#endif
	move.l	DOSBase(%sp),%a6	/* Get SysBase */
	move.l	dl_SysBase(%a6),%a6	/* "   " */
#if !UseRegisterArgs
	move.l	%a6,-8(%a1)		/* Push SysBase onto the new stack */
	add.l	#-16,%a1
#else
	move.l	%a6,-(%a1)		/* Push SysBase onto the new stack */
#endif
	move.l	%a1,stk_Pointer(%a0)	/* Store Switch Point in sss */

#if !UseRegisterArgs
	/* Push SysBase and sss on our stack */
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#endif

	jsr	StackSwap(%a6)		/* Switch stacks (a0=sss) */

#if !UseRegisterArgs
	/* Clean new stack from call to StackSwap */
	addq.w	#8,%sp
#else
	/*
            The entry function expects to be called with:
            argptr -> a0
            arglen -> d0
            execbase -> a6
        */
	move.l	%a2,%a0			/* argptr in a0 */
	move.l	%d2,%d0			/* arglen in d0 */
#endif

	jsr	(%a5)			/* Call the specified routine */

#if !UseRegisterArgs
	/* Clean (new) stack partially, leaving SysBase behind */
	addq.w	#8,%sp
#endif
	move.l	%d0,%a5			/* Save return value */

	/* Swap the upper two values on the new stack */
	move.l	(%sp)+,%a6		/* Pop SysBase */
	move.l	(%sp)+,%a0		/* Pop sss */
#if !UseRegisterArgs
	move.l	%a6,-(%sp)
	move.l	%a0,-(%sp)
#endif

	jsr	StackSwap(%a6)		/* Switch stacks back */

#if !UseRegisterArgs
	/* Clean old stack */
	addq.w	#8,%sp
#endif
	move.l	%a5,%d0			/* Put result in d0 where our caller expects it */

#if !UseRegisterArgs
	movem.l	(%sp)+,%a5-%a6		/* Restore registers */
#else
	movem.l (%sp)+,%d2/%a2/%a5-%a6	/* Restore registers */
#endif
	rts
