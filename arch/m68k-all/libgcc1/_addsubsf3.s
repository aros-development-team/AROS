| single floating point add/subtract routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
|
| Revision 1.3, kub 01-90 :
| added support for denormalized numbers
|
| Revision 1.2, kub 01-90 :
| replace far shifts by swaps to gain speed (more optimization is of course
| possible by doing shifts all in one intruction, but what about the rounding
| bits)
|
| Revision 1.1, kub 12-89 :
| Created single float version for 68000
|
| Revision 1.0:
| original 8088 code from P.S.Housel for double floats

	.text
	.even
	.globl	__subsf3
	.globl	__addsf3

__subsf3:
	eorb	#0x80,%sp@(8)	| reverse sign of v
__addsf3:
	lea	%sp@(4),%a0	| pointer to u and v parameter
	moveml	%d2-%d5,%sp@-	| save registers
	moveml	%a0@,%d4/%d5	| %d4 = v, %d5 = u

	movel	%d5,%d0		| %d0 = u.exp
	swap	%d0
	movel	%d5,%d2		| %d2.h = u.sign
	movew	%d0,%d2
	lsrw	#7,%d0
	andw	#0xff,%d0	| kill sign bit (exponent is 8 bits)

	movel	%d4,%d1		| %d1 = v.exp
	swap	%d1
	eorw	%d1,%d2		| %d2.l = u.sign ^ v.sign
	lsrw	#7,%d1
	andw	#0xff,%d1	| kill sign bit (exponent is 8 bits)

	andl	#0x7fffff,%d5	| remove exponent from mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_00
	orl	#0x800000,%d5	| restore implied leading "1"
	bra	L_10
L_00:	addw	#1,%d0		| "normalize" exponent
L_10:
	andl	#0x7fffff,%d4	| remove exponent from mantissa
	tstw	%d1		| check for zero exponent - no leading "1"
	beq	L_01
	orl	#0x800000,%d4	| restore implied leading "1"
	bra	L_11
L_01:	addw	#1,%d1		| "normalize" exponent
L_11:
	clrw	%d3		| (put initial zero rounding bits in %d3)
	negw	%d1		| %d1 = u.exp - v.exp
	addw	%d0,%d1
	beq	L_5		| exponents are equal - no shifting neccessary
	bgt	L_12		| not equal but no exchange neccessary
	exg	%d4,%d5		| exchange u and v
	subw	%d1,%d0		| %d0 = u.exp - (u.exp - v.exp) = v.exp
	negw	%d1
	tstw	%d2		| %d2.h = u.sign ^ (u.sign ^ v.sign) = v.sign
	bpl	L_12
	bchg	#31,%d2
L_12:
	cmpw	#24,%d1		| is u so much bigger that v is not
	bge	L_7		| significant ?

	movew	#7-1,%d3		| shift u left up to 7 bits to minimize loss
L_2:
	addl	%d5,%d5
	subw	#1,%d0		| decrement exponent
	subw	#1,%d1		| done shifting altogether ?
	dbeq	%d3,L_2		| loop if still can shift u.mant more
	clrw	%d3

	cmpw	#16,%d1		| see if fast rotate possible
	blt	L_4
	orb	%d4,%d3		| set rounding bits
	orb	%d2,%d3
	sne	%d2		| "sticky byte"
	movew	%d4,%d3
	lsrw	#8,%d3
	clrw	%d4		| rotate by swapping register halfs
	swap	%d4
	subw	#16,%d1
L_02:
	lsrl	#1,%d4		| shift v.mant right the rest of the way
	orb	%d3,%d2		| set "sticky byte" if necessary
	roxrw	#1,%d3		| shift into rounding bits
L_4:	dbra	%d1,L_02		| loop
	andb	#1,%d2		| see if "sticky bit" should be set
	orb	%d2,%d3
L_5:
	tstw	%d2		| are the signs equal ?
	bpl	L_6		| yes, no negate necessary

	negb	%d3		| negate rounding bits and v.mant
	negl	%d4
L_6:
	addl	%d4,%d5		| u.mant = u.mant + v.mant
	bcs	L_7		| needn't negate
	tstw	%d2		| opposite signs ?
	bpl	L_7		| don't need to negate result

	negb	%d3		| negate rounding bits and u.mant
	negl	%d5
	notl	%d2		| switch sign
L_7:
	movel	%d5,%d4		| move result for normalization
	moveb	%d3,%d1		| put rounding bits in %d1 for norm_sf
	swap	%d2		| put sign into %d2 (exponent is in %d0)
	jmp	norm_sf		| leave registers on stack for norm_sf
