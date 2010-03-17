	.text
	.even
	.globl __nedf2

__nedf2:
	moveml	%sp@(4),%d0-%d1/%a0-%a1
	cmpl	%d0,%a0
	sne	%d0
	tstb	%d0
	bne	L_1
	cmpl	%d1,%a1
	sne	%d0
L_1:	andl	#1,%d0
	rts
