| long integer to single float conversion routine
|

BIAS4	=	0x7F-1

	.text
	.even
	.globl	__floatsisf

__floatsisf:
	movel	%sp@(4),%d0	| get the 4-byte integer
	moveml	%d2-%d5,%sp@-	| save registers to make norm_sf happy

	movel	%d0,%d4		| prepare result mantissa
	clrl	%d5
	movew	#BIAS4+32-8,%d0	| radix point after 32 bits
L_0:
	movel	%d4,%d2		| set sign flag
	swap	%d2
	tstw	%d2		| check sign of number
	bge	L_1		| nonnegative
	negl	%d4		| take absolute value
L_1:
	clrw	%d1		| set rounding = 0
	jmp	norm_sf
