/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: Run a process ( invoked by dos/Runcommand() )
 
  LONG RunProcess ( struct Process         * proc,
 		    struct StackSwapStruct * sss,
 		    STRPTR		     argptr,
 		    ULONG		     argsize,
 		    LONG_FUNC		     entry,
 		    struct DosLibrary	   * DOSBase
*/

	#include "machine.i"

	# Stackframe
	FirstArg	= 4+(2*4)	/* Return-address + registers */
	proc		= FirstArg
	sss		= proc+4
	argptr		= sss+4
	argsize		= argptr+4
	entry		= argsize+4
	DOSBase		= entry+4

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(RunProcess,Dos)
	.type	AROS_SLIB_ENTRY(RunProcess,Dos),@function
AROS_SLIB_ENTRY(RunProcess,Dos):
	movem.l	a5-a6,-(sp)		/* Save some registers */

	move.l	sss(sp),a0		/* Fetch the arguments off the stack */
	move.l	entry(sp),a5		/* "     "   "         "   "   " */

	move.l	stk_Upper(a0),a1	/* Move upper bounds of the new stack into a1 */
	move.l	a0,-(a1)		/* Push sss onto the new stack */
	move.l	DOSBase(sp),a6		/* Get SysBase */
	move.l	dl_SysBase(a6),a6	/* "   " */
	move.l	a6,-(a1)		/* Push SysBase onto the new stack */
	move.l	a1,stk_Pointer(a0)	/* Store Switch Point in sss */

	jsr	StackSwap(a6)		/* Switch stacks (a0=sss) */

	jsr	(a5)			/* Call the specified routine */
	move.l	d0,a5			/* Save return value */

	move.l	(sp)+,a6		/* Pop SysBase off the new stack */
	move.l	(sp)+,a0		/* Pop sss off the new stack */
	jsr	StackSwap(a6)		/* Switch stacks back */

	move.l	a5,d0			/* Put result in d0 where our caller expects it */

	movem.l	(sp)+,a5-a6		/* Restore registers */
	rts
