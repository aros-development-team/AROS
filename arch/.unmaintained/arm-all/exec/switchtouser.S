/*   Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

	/*
	 * Switch the processor into user mode.
	 * Parameters to this function:
	 *   function to continue with afterwards
	 *   pointer to the user stack to be used
	 */
	.text
	.align	2
	.globl	switch_to_user_mode
	.type	switch_to_user_mode,function

switch_to_user_mode:
	/*
	 * r0: function where to continue afterwards.
	 * r1: user stack pointer
	 */
	/*
	 * Load the user stack pointer with its future value!
	 */
	stmia	sp, {r1}
	ldmia	sp, {sp}^
	/*
	 * Make sure we are in SVC mode
	 * Disable all interrupts
	 */
	mrs	r1, cpsr
	bic	r1, r1, #0xff
	orr	r1, r1,#(0x80|0x40|0x13)
	msr	cpsr, r1
	/*
	 * Do the actual switch into user mode
	 * Disable all interrupts: 0x80, 0x40
	 * Set User Mode: 0x10
	 */
	mrs	r1, cpsr
	bic	r1, r1, #0xff
	orr	r1, r1, #(0x80|0x40|0x10)
	msr	spsr, r1
	/*
	 * Call the follow up function
	 */
	movs	pc, r0
