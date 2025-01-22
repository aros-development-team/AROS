/*   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	/*
	 * Switch the processor into user mode.
	 * Parameters to this function:
	 *   function to continue with afterwards.
	 *   pointer to the user stack to be used.
	 */
	.text
	.balign 16
	.globl	switch_to_user_mode

switch_to_user_mode:
	move.l	0x8(%sp),%a0	// get USP
	move.l	%a0,%usp        // set the USP to the new value

	move.l	%ssp,%a1        // ssp->a1 for later to access

	lea.l	in_user_mode,%a0// prepare a fake exception stack frame
	move.l	%a0,-(%ssp)	// PC
	move.w	#0x0700,-(%ssp)	// SR
	rte			// get the PC and SR from the exception
	                        // stack frame
in_user_mode:
	move.l	0x4(%a1),%a0	// a1 contains old ssp
	jmp	(%a0)           // jump to address where to continue
