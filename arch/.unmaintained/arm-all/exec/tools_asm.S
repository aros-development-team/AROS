	.align	2
	.globl	getsr
	.type	getsr, function
getsr:
	mrs	r0, CPSR
	mov	pc, lr

	.align	2
	.globl	getr9
	.type	getr9, function
getr9:
	mov	r0, r9
	mov	pc, lr

	.align	2
	.globl	getsp
	.type	getsp, function
getsp:
	mov	r0, sp
	mov	pc, lr

	.align	2
	.globl	get_cp15_r0
	.type	get_cp15_r0, function
get_cp15_r0:
	mrc	p15, 0, %r0, c0, c0
	nop
	nop
	nop
	nop
	mov	pc, lr

	.align	2
	.globl	get_cp15_r1
	.type	get_cp15_r1, function
get_cp15_r1:
	mrc	p15, 0, %r0, c1, c0
	nop
	nop
	nop
	nop
	mov	pc, lr

	.align	2
	.globl	get_cp15_r2
	.type	get_cp15_r2, function
get_cp15_r2:
	mrc	p15, 0, %r0, c2, c0
	nop
	nop
	nop
	nop
	mov	pc, lr
