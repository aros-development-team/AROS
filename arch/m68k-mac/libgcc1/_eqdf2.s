	.text
	.even
	.globl __eqdf2

__eqdf2:
	moveml	%sp@(4),%d0-%d1/%a0-%a1
	cmpl	%d0,%a0
	seq	%d0
	tstb	%d0
	beq	L_1
	cmpl	%d1,%a1
	seq	%d0
L_1:	notb	%d0
	andl	#1,%d0
	rts
