| single float to long conversion routine
|

BIAS4	=	0x7F-1

	.text
	.even
	.globl	__fixsfsi

__fixsfsi:
	lea	%sp@(4),%a0	| pointer to parameters
	moveml	%d2/%d4/%d5,%sp@-	| save registers
	movel	%a0@,%d4		| get the number
	movew	%a0@,%d0		| extract exp
	movew	%d0,%d2		| extract sign
	lsrw	#7,%d0
	andw	#0xff,%d0	| kill sign bit

	andl	#0x7fffff,%d4	| remove exponent from mantissa
	orl	#0x800000,%d4	| restore implied leading "1"

	cmpw	#BIAS4,%d0	| check exponent
	blt	zero		| strictly factional, no integer part ?
	cmpw	#BIAS4+32,%d0	| is it too big to fit in a 32-bit integer ?
	bgt	toobig

	subw	#BIAS4+24,%d0	| adjust exponent
	bgt	L_2		| shift up
	beq	L_3		| no shift

	cmpw	#-8,%d0		| replace far shifts by swap
	bgt	L_1
	movew	%d4,%d5		| shift fast, 16 bits
	swap	%d5
	clrw	%d4
	swap	%d4
	addw	#16,%d0		| account for swap
	bgt	L_2
	beq	L_3

L_1:	lsrl	#1,%d4		| shift down to align radix point;
	addw	#1,%d0		| extra bits fall off the end (no rounding)
	blt	L_1		| shifted all the way down yet ?
	bra	L_3

L_2:	addl	%d5,%d5		| shift up to align radix point
	addxl	%d4,%d4
	subw	#1,%d0
	bgt	L_2

L_3:	movel	%d4,%d0		| put integer into result register
	cmpl	#0x80000000,%d0	| -2147483648 is a nasty evil special case
	bne	L_6
	tstw	%d2		| this had better be -2^31 and not 2^31
	bpl	toobig
	bra	L_8
L_6:	tstl	%d0		| sign bit set ? (i.e. too big)
	bmi	toobig
L_7:
	tstw	%d2		| is it negative ?
	bpl	L_8
	negl	%d0		| negate
L_8:
	moveml	%sp@+,%d2/%d4/%d5
	rts

zero:
	clrl	%d0		| make the whole thing zero
	bra	L_7

toobig:
	movel	#0x7fffffff,%d0	| ugh. Should cause a trap here.
	bra	L_7
