/*   Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

	
	.data
	.globl	jumptable
	.align	4
	.type	jumptable,object
	.size	jumptable,16
jumptable:
	.word	sys_Dispatch         // d7 = 0
	.word	_sys_SetSR            // d7 = 4
	.word	0 // _sys_Supervisor       // d7 = 8
	.word	0 //_sys_SuperState       // d7 = 12




	.text
	.align	2
	.globl	_sys_swi_handler
	.type	_sys_swi_handler,function

_sys_swi_handler:
	sub	sp, sp, #(17*4)
	stmia	sp, {r0-r12}
	add	r8, sp, #(13*4)
	stmia	r8, {sp, lr}^
	nop
	str	lr, [sp, #(15*4)]
	mrs	r8, spsr
	str	r8, [sp, #(16*4)]

	ldr	r3, [lr, #-4]
	bic	r3, r3, #0xff000000
	ldr	r2, .jumptable


	mov	r0, sp
	mov	r1, #-4   // adjustment factor for regs->lr_svc due to swi
	mov	lr, pc
	ldr	pc, [r2, r3, lsl #2]
	nop
	ldr	r0, [sp, #(16*4)]
	msr	spsr, r0
	ldr	lr, [sp, #(15*4)]
	ldmia	sp, {r0-lr}^
	nop

	add	sp,sp,#(17*4)

	movs	pc, lr

	.globl	.jumptable
	.align	4
	.type	.jumptable,object
	.size	.jumptable,4
.jumptable:
	.word	jumptable
