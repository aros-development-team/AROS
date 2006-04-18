	.text
	.even
	.globl __gtdf2

__gtdf2:
	moveml	%sp@(4),%d0-%d1/%a0-%a1 | get u and v
	tstl	%d0		| check sign bit
	bpl	L_1
	negl	%d1		| negate
	negxl	%d0
	eorl	#0x80000000,%d0	| toggle sign bit
L_1:
	exg	%a0,%d0
	exg	%a1,%d1
	tstl	%d0		| check sign bit
	bpl	L_2
	negl	%d1		| negate
	negxl	%d0
	eorl	#0x80000000,%d0	| toggle sign bit
L_2:
	cmpl	%d0,%a0
	blt	le
	bgt	gt
	cmpl	%d1,%a1
	bls	le
gt:
	moveq	#1,%d0
	rts
le:
	clrl	%d0
	rts
