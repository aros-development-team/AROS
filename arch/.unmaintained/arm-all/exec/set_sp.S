	/*
	 * Set the stack pointers
	 */
	.text
	.align	2
	.globl	set_sp_mode
	.type	set_sp_mode,function
set_sp_mode:
	/*
	 * r0: stack pointer for given mode
         * r1: mode, bits 4:0 of status register
	 */
	/*
	 * Switch to requested mode
	 * Disable all interrupts
	 */
	msr	cpsr_c, r1

	/*
	 * Set the stack pointer for requested mode
	 */
	mov	sp, r0
	/*
	 * Switch to SVC mode
	 * Disable all interrupts
	 */
	mov	r1, #(0x80|0x40|0x13)
	msr	cpsr_c, r1

	mov	pc, lr
