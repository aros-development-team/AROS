/*
     (C) 1995-96 AROS - The Amiga Replacement OS
     $Id$
 
     Desc:
     Lang:
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

#include "machine.i"

	.text
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)
	.type	AROS_SLIB_ENTRY(StackSwap,Exec),@function
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

	movl	(4),%eax
	/* Reenable interrupts. */
	call	*Enable(%eax)
	popl	%eax

	/* Restore returnaddress and return */
	pushl	%edx
	ret

