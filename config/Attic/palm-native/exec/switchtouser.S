/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	/*
	 * Switch the processor into user mode.
	 */
	.text
	.balign 16
	.globl	switch_to_user_mode

switch_to_user_mode:
	move.l	0x1004,%a0
	move.l	%a0,%usp
	move.l	%ssp,%a1
	lea.l	10(%pc),%a0 /* cannot use another addressing mode here due
                               to the stupid compiler gets address of 
                               in_user_mode! */
	move.l	%a0,-(%ssp)
	move.w	#0x0000,-(%ssp)
	rte
in_user_mode:
	move.l	0x4(%a1),%a0
	jmp	(%a0)
