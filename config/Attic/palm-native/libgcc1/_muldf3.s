| double floating point multiplication routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
|
| Revision 1.2, kub 01-90 :
| added support for denormalized numbers
|
| Revision 1.1, kub 12-89 :
| Ported over to 68k assembler
|
| Revision 1.0:
| original 8088 code from P.S.Housel

BIAS8	=	0x3FF-1

	.text
	.even
	.globl	__muldf3

__muldf3:
	lea	%sp@(4),%a0
	moveml	%d2-%d7,%sp@-
	moveml	%a0@,%d4-%d5/%d6-%d7 | %d4-%d5 = v, %d6-%d7 = u
	subw	#16,%sp		| multiplication accumulator

	movel	%d6,%d0		| %d0 = u.exp
	swap	%d0
	movew	%d0,%d2		| %d2 = u.sign
	lsrw	#4,%d0
	andw	#0x07ff,%d0	| kill sign bit

	movel	%d4,%d1		| %d1 = v.exp
	swap	%d1
	eorw	%d1,%d2		| %d2 = u.sign ^ v.sign (in bit 31)
	lsrw	#4,%d1
	andw	#0x07ff,%d1	| kill sign bit

	andl	#0x0fffff,%d6	| remove exponent from u.mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_00
	orl	#0x100000,%d6	| restore implied leading "1"
	bra	L_10
L_00:	addw	#1,%d0		| "normalize" exponent
L_10:	movel	%d6,%d3
	orl	%d7,%d3
	beq	retz		| multiplying by zero

	andl	#0x0fffff,%d4	| remove exponent from v.mantissa
	tstw	%d1		| check for zero exponent - no leading "1"
	beq	L_01
	orl	#0x100000,%d4	| restore implied leading "1"
	bra	L_11
L_01:	addw	#1,%d1		| "normalize" exponent
L_11:	movel	%d4,%d3
	orl	%d5,%d3
	beq	retz		| multiplying by zero

	addw	%d1,%d0		| add exponents,
	subw	#BIAS8+16-11,%d0	| remove excess bias, acnt for repositioning

	clrl	%sp@		| initialize 128-bit product to zero
	clrl	%sp@(4)
	clrl	%sp@(8)
	clrl	%sp@(12)
	lea	%sp@(8),%a1	| accumulator pointer

| see Knuth, Seminumerical Algorithms, section 4.3. algorithm M

	swap	%d2
	movew	#4-1,%d2
L_12:
	movew	%d5,%d3
	mulu	%d7,%d3		| mulitply with bigit from multiplier
	addl	%d3,%a1@(4)	| store into result
	movew	%d4,%d3
	mulu	%d7,%d3
	movel	%a1@,%d1		| add to result
	addxl	%d3,%d1
	movel	%d1,%a1@
	roxlw	%a1@-		| rotate carry in

	movel	%d5,%d3
	swap	%d3
	mulu	%d7,%d3
	addl	%d3,%a1@(4)	| add to result
	movel	%d4,%d3
	swap	%d3
	mulu	%d7,%d3
	movel	%a1@,%d1		| add to result
	addxl	%d3,%d1
	movel	%d1,%a1@

	movew	%d6,%d7
	swap	%d6
	swap	%d7
	dbra	%d2,L_12

	swap	%d2		| [TOP 16 BITS SHOULD BE ZERO !]

	moveml	%sp@(2),%d4-%d7	| get the 112 valid bits
	clrw	%d7		| (pad to 128)
L_2:
	cmpl	#0x0000ffff,%d4	| multiply (shift) until
	bhi	L_3		|  1 in upper 16 result bits
	cmpw	#9,%d0		| give up for denormalized numbers
	ble	L_3
	swap	%d4		| (we're getting here only when multiplying
	swap	%d5		|  with a denormalized number; there's an
	swap	%d6		|  eventual loss of 4 bits in the rounding
	swap	%d7		|  byte -- what a pity 8-)
	movew	%d5,%d4
	movew	%d6,%d5
	movew	%d7,%d6
	clrw	%d7
	subw	#16,%d0		| decrement exponent
	bra	L_2
L_3:
	movel	%d6,%d1		| get rounding bits
	roll	#8,%d1
	movel	%d1,%d3		| see if sticky bit should be set
	orl	%d7,%d3		| (lower 16 bits of %d7 are guaranteed to be 0)
	andl	#0xffffff00,%d3
	beq	L_4
	orb	#1,%d1		| set "sticky bit" if any low-order set
L_4:	addw	#16,%sp		| remove accumulator from stack
	jmp	norm_df		| (result in %d4/%d5)

retz:	clrl	%d0		| save zero as result
	clrl	%d1
	addw	#16,%sp
	moveml	%sp@+,%d2-%d7
	rts			| no normalizing neccessary
