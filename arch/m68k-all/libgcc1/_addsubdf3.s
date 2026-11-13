| double floating point add/subtract routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
|
| Revision 1.3, kub 01-90 :
| added support for denormalized numbers
|
| Revision 1.2, kub 01-90 :
| replace far shifts by swaps to gain speed
|
| Revision 1.1, kub 12-89 :
| Ported over to 68k assembler
|
| Revision 1.0:
| original 8088 code from P.S.Housel

	.text
	.even
	.globl	__subdf3
	.globl	__adddf3

__subdf3:
	eorb	#0x80,%sp@(12)	| reverse sign of v
__adddf3:
	lea	%sp@(4),%a0	| pointer to u and v parameter
	moveml	%d2-%d7,%sp@-	| save registers
	moveml	%a0@,%d4-%d5/%d6-%d7	| %d4-%d5 = v, %d6-%d7 = u

	movel	%d6,%d0		| %d0 = u.exp
	swap	%d0
	movel	%d6,%d2		| %d2.h = u.sign
	movew	%d0,%d2
	lsrw	#4,%d0
	andw	#0x07ff,%d0	| kill sign bit

	movel	%d4,%d1		| %d1 = v.exp
	swap	%d1
	eorw	%d1,%d2		| %d2.l = u.sign ^ v.sign
	lsrw	#4,%d1
	andw	#0x07ff,%d1	| kill sign bit

	andl	#0x0fffff,%d6	| remove exponent from u.mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_00
	orl	#0x100000,%d6	| restore implied leading "1"
	bra	L_10
L_00:	addw	#1,%d0		| "normalize" exponent
L_10:
	andl	#0x0fffff,%d4	| remove exponent from v.mantissa
	tstw	%d1		| check for zero exponent - no leading "1"
	beq	L_01
	orl	#0x100000,%d4	| restore implied leading "1"
	bra	L_11
L_01:	addw	#1,%d1		| "normalize" exponent
L_11:
	clrw	%d3		| (put initial zero rounding bits in %d3)
	negw	%d1		| %d1 = u.exp - v.exp
	addw	%d0,%d1
	beq	L_5		| exponents are equal - no shifting neccessary
	bgt	L_12		| not equal but no exchange neccessary
	exg	%d4,%d6		| exchange u and v
	exg	%d5,%d7
	subw	%d1,%d0		| %d0 = u.exp - (u.exp - v.exp) = v.exp
	negw	%d1
	tstw	%d2		| %d2.h = u.sign ^ (u.sign ^ v.sign) = v.sign
	bpl	L_12
	bchg	#31,%d2
L_12:
	cmpw	#53,%d1		| is u so much bigger that v is not
	bge	L_7		| significant ?

	movew	#10-1,%d3	| shift u left up to 10 bits to minimize loss
L_2:
	addl	%d7,%d7
	addxl	%d6,%d6
	subw	#1,%d0		| decrement exponent
	subw	#1,%d1		| done shifting altogether ?
	dbeq	%d3,L_2		| loop if still can shift u.mant more
	clrw	%d3
L_3:
	cmpw	#16,%d1		| see if fast rotate possible
	blt	L_4
	orb	%d5,%d3		| set rounding bits
	orb	%d2,%d3
	sne	%d2		| "sticky byte"
	movew	%d5,%d3
	lsrw	#8,%d3
	movew	%d4,%d5		| rotate by swapping register halfs
	swap	%d5
	clrw	%d4
	swap	%d4
	subw	#16,%d1
	bra	L_3
L_0:
	lsrl	#1,%d4		| shift v.mant right the rest of the way
	roxrl	#1,%d5		| to line it up with u.mant
	orb	%d3,%d2		| set "sticky byte" if necessary
	roxrw	#1,%d3		| shift into rounding bits
L_4:	dbra	%d1,L_0		| loop
	andb	#1,%d2		| see if "sticky bit" should be set
	orb	%d2,%d3
L_5:
	tstw	%d2		| are the signs equal ?
	bpl	L_6		| yes, no negate necessary

	negb	%d3		| negate rounding bits and v.mant
	negl	%d5
	negxl	%d4
L_6:
	addl	%d5,%d7		| u.mant = u.mant + v.mant
	addxl	%d4,%d6
	bcs	L_7		| needn't negate
	tstw	%d2		| opposite signs ?
	bpl	L_7		| don't need to negate result

	negb	%d3		| negate rounding bits and u.mant
	negl	%d7
	negxl	%d6
	notl	%d2		| switch sign
L_7:
	movel	%d6,%d4		| move result for normalization
	movel	%d7,%d5
	moveb	%d3,%d1		| put rounding bits in %d1 for norm_df
	swap	%d2		| put sign into %d2 (exponent is in %d0)
	jmp	norm_df		| leave registers on stack for norm_df
