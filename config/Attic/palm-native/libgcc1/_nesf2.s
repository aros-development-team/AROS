	.text
	.even
	.globl __nesf2

__nesf2:
	moveml	%sp@(4),%d0-%d1
	cmpl	%d0,%d1
	sne	%d0
	andl	#1,%d0
	rts

