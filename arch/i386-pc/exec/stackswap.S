/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************
 
    NAME
 
 	AROS_LH1(void, StackSwap,
 
    SYNOPSIS
 	AROS_LHA(struct StackSwapStruct *, newStack, A0),
 
    LOCATION
 	struct ExecBase *, SysBase, 122, Exec)
 
    FUNCTION
 	This function switches to the new stack given by the parameters in the
 	stackswapstruct structure. The old stack parameters are returned in
 	the same structure so that the stack can be restored later
 
    INPUTS
 	newStack - parameters for the new stack
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
    HISTORY
 
******************************************************************************/

#include "include/machine.i"

	.text
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)
	_FUNCTION(AROS_SLIB_ENTRY(StackSwap,Exec))
AROS_SLIB_ENTRY(StackSwap,Exec):
	/* Preserve returnaddress and fix sp */
	popl	%edx
	movl	4(%esp),%eax

	/* Get pointer to tc_SPLower in a1 (tc_SPUpper is next) */
	movl	ThisTask(%eax),%ecx
	leal	tc_SPLower(%ecx),%ecx

	/* Just to be sure interrupts always find a good stackframe */
	pushl	%eax
	call	*Disable(%eax)
	popl	%eax

	movl	(%esp),%eax
	pushl	%ebx

	/* Swap Lower boundaries */
	movl	(%eax),%ebx
	xchgl	%ebx,(%ecx)
	movl	%ebx,(%eax)

	addl	$4,%eax
	addl	$4,%ecx

	/* Swap higher boundaries */
	movl	(%eax),%ebx
	xchgl	%ebx,(%ecx)
	movl	%ebx,(%eax)

	addl	$4,%eax

	/* Swap stackpointers */
	popl	%ebx
	xchgl	(%eax),%esp

	pushl %edx
	movl	(4),%eax
	/* Reenable interrupts. */
	pushl	%eax
	call	*Enable(%eax)
	popl	%eax
//	popl	%edx

	/* Restore returnaddress and return */
//	pushl	%edx
	ret

