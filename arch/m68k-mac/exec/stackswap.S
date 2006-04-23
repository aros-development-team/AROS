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

	#include "../include/aros/machine.i"

	.text
	.balign 4
	.globl	AROS_SLIB_ENTRY(StackSwap,Exec)

AROS_SLIB_ENTRY(StackSwap,Exec):

	// Here's how the stack looks right now
	// 0(%sp) : Return Address
#ifndef DoRegisterCalls
	// 4(%sp) : pointer to StackSwap structure
#endif

	/* Preserve returnaddress and fix sp */
	move.l	(%sp)+,%d1       // d1 contains return address
#ifndef DoRegisterCalls
	move.l	(%sp)+,%a0       // a0 holds param to this function
#endif
	                         // now the stack is 'clean'

	move.l  %a6,-(%sp)       // save contents of %a6

	/* Get pointer to tc_SPLower in a1 (tc_SPUpper is next) */
	move.l	0x4,%a6          // execbase to a6
	bsr	get_offsetof_ThisTask
	move.l	(%a6,%d0),%a1
	bsr	get_offsetof_tc_SPLower
	lea.l	(%a1,%d0),%a1

	move.l	%d1,%d0          // d0 contains return address now


	movem.l	%d0/%a0/%a1,-(%sp) // d0: save return address on stack
                                   // a0: param to this function onto stack
	                           // a1: address of tc_SPLower to stack
	move.l  %a6,-(%sp)         // a6: Execbase onto stack
	
	/* Just to be sure interrupts always find a good stackframe */
	jsr	Disable(%a6)     // disable interrupts

	move.l  (%sp)+,%a6     	 // a6: Execbase from stack
 	movem.l	(%sp)+,%d0/%a0/%a1  // a1 holds address of tc_SPlower
	                            // a0 holds param to this function
                                    // d0 holds return address
	
	move.l  (%sp)+,%a6     	 // original a6 from stack

	/* Swap Lower boundaries */
	move.l	(%a1),%d1        // d1 holds content of tc_SPLower
	move.l	(%a0),(%a1)+     // write the content of the first param in
                                 // the structure to this function to tc_SPlower
	move.l	%d1,(%a0)+       // write d1 back to structure

	/* Swap higher boundaries */
	move.l	(%a1),%d1        // d1 holds content of tc_SPHigher (???? check)
	move.l	(%a0),(%a1)      // write the content of the 2nd param in
                                 // the structure to this function to tc_SPHigher (??? check)
	move.l	%d1,(%a0)+       // write d1 back to structure

	/* Swap stackpointers */
	move.l	%sp,%d1          // current stack pointer to d1
	move.l	(%a0),%sp        // desired stack pointer to sp
	move.l	%d1,(%a0)        // old stack pointer back into the structure

    	movem.l	%d0/%a6,-(%sp)   // save %d0 onto stack
	                         // save %a6 onto stack
	
	/* Reenable interrupts. */
	move.l  0x4,%a6          // get ExecBase
	move.l  %a6,-(%sp)       // ExecBase onto stack
	jsr	Enable(%a6)      // Enable interrupts
	move.l  (%sp)+,%a6       // dummy

	movem.l	(%sp)+,%d0/%a6       // restore orig. content of %a6 register
	                             // restore return address to d0

#ifndef DoRegisterCalls
	move.l	#0,-(%sp)        // write fake parameter to this function
	                         // onto stack
#endif
	/* Restore returnaddress and return */
	move.l	%d0,-(%sp)       // return address onto stack 

	// Here's how the stack looks right now
	// 0(%sp) : Return Address
	// 4(%sp) : fake pointer to StackSwap structure
	rts
