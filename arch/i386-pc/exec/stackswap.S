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
#include <aros/config.h>

	.text
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)
	_FUNCTION(AROS_SLIB_ENTRY(StackSwap,Exec))
AROS_SLIB_ENTRY(StackSwap,Exec):
	/* Save %ebx content */
	pushl	%ebx
	
	/* Take the function's parameters */
	movl	8(%esp),  %ebx  //newStack
	movl	12(%esp), %eax  //SysBase

	/* Disable interrupts, to be sure they always find a good stackframe */
	pushl	%eax
	call	*Disable(%eax)
	popl	%eax

	/* FindTask(NULL) in %edx */
	movl	ThisTask(%eax), %edx
	
	/* Swap Lower boundaries, %ecx is used as temp register */
	movl    tc_SPLower(%edx), %ecx
	xchgl	%ecx,             stk_Lower(%ebx)
	movl    %ecx,             tc_SPLower(%edx)

	/* Swap Upper boundaries, %ecx is used as temp register */
	movl    tc_SPUpper(%edx), %ecx
	xchgl	%ecx,             stk_Upper(%ebx)
	movl    %ecx,             tc_SPUpper(%edx)
	
#if AROS_STACK_DEBUG
	/* Fill [stk_Lower .. stk_Pointer - 16] with 0xE1 */
	
	pushl	%edi
	pushl	%eax
	
	/* Destination register = %edi = NEW SPLower.
	   Which was already swapped above, so to be found
	   in task->tc_SPLower */
	movl	tc_SPLower(%edx), %edi

	/* %ecx = count register = NEW SP register - NEW SP_Lower - 16 */
	movl	stk_Pointer(%ebx), %ecx
	subl	$16, %ecx
	subl	%edi, %ecx
	
	/* byte value to store */
	movb	$0xE1, %al
	
	/* direction to store: forward */
	cld
	
	/* do the store operation: put %ecx times %al into memory starting at %edi. */
	rep
	stosb
	
	popl	%eax
	popl	%edi
#endif	

	/* We need to restore %ebx before swapping stack pointers, so
	   put its content in a register whose content doesn't matter
	   anymore to us, like %edx.  */
	movl    %ebx, %edx  // newStack, ThisTask is lost

	/* Now restore %ebx */
	popl	%ebx        
	
	/* And take the function's return address, before we
	   swap stacks.  */
	popl    %ecx    // return address    
	
	/* Swap stack pointers */
	xchgl	%esp, stk_Pointer(%edx) 

	/* Reenable interrupts. */
	pushl	%eax
	call	*Enable(%eax)
	popl	%eax
	
	/* Return. %ecx contains the return address, and the stack is already
	   adjusted, we don't need, or even can, use ret here.  */
        jmp	*%ecx

