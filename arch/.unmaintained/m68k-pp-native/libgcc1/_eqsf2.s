	.text
	.even
	.globl __eqsf2

__eqsf2:
	moveml	%sp@(4),%d0-%d1
	cmpl	%d0,%d1
	seq	%d0
	notb	%d0
	andl	#1,%d0
	rts
