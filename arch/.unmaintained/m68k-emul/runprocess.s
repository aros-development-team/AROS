#    (C) 1995-96 AROS - The Amiga Replacement OS
#    $Id$
#    $Log$
#    Revision 1.1  1996/12/05 15:31:00  aros
#    Patches by Geert Uytterhoeven integrated
#
#    Revision 1.1  1996/11/01 02:03:41  aros
#    Run a process (invoked by dos/RunCommand)
#
#
#    Desc: Run a process ( invoked by dos/Runcommand() )
#    Lang:
#
# LONG RunProcess ( struct Process         * proc,
#		    struct StackSwapStruct * sss,
#		    STRPTR		     argptr,
#		    ULONG		     argsize,
#		    LONG_FUNC		     entry,
#		    struct DosLibrary	   * DOSBase

	.include "machine.i"

	# Stackframe
	FirstArg	= 4+(2*4)	/* Return-address + registers */
	proc		= FirstArg
	sss		= proc+4
	argptr		= sss+4
	argsize		= argptr+4
	entry		= argsize+4
	DOSBase		= entry+4

	.text
	.balign 16
	.globl	_Dos_RunProcess
	.type	_Dos_RunProcess,@function
_Dos_RunProcess:
	moveml	%a5-%a6,%sp@-		/* Save some registers */

	movel	%sp@(sss),%a0		/* Fetch the arguments off the stack */
	movel	%sp@(entry),%a5		/* "     "   "         "   "   " */

	movel	%a0@(stk_Upper),%a1	/* Move upper bounds of the new stack into a1 */
	movel	%a0,%a1@-		/* Push sss onto the new stack */
	movel	%sp@(DOSBase),%a6	/* Get SysBase */
	movel	%a6@(dl_SysBase),%a6	/* "   " */
	movel	%a6,%a1@-		/* Push SysBase onto the new stack */
	movel	%a1,stk_Pointer(%a0)	/* Store Switch Point in sss */

	jsr	%a6@(StackSwap)		/* Switch stacks (a0=sss) */

	jsr	%a5@			/* Call the specified routine */
	movel	%d0,%a5			/* Save return value */

	movel	%sp@+,%a6		/* Pop SysBase off the new stack */
	movel	%sp@+,%a0		/* Pop sss off the new stack */
	jsr	%a6@(StackSwap)		/* Switch stacks back */

	movel	%a5,%d0			/* Put result in d0 where our caller expects it */

	moveml	%sp@+,%a5-%a6		/* Restore registers */
	rts
