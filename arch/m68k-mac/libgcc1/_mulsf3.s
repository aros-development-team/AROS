| single floating point multiplication routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
|
| Revision 1.2, kub 01-90 :
| added support for denormalized numbers
|
| Revision 1.1, kub 12-89 :
| Created single float version for 68000. Code could be speed up by having
| the accumulator in the 68000 register set ...
|
| Revision 1.0:
| original 8088 code from P.S.Housel for double floats

BIAS4	=	0x7F-1

	.text
	.even
	.globl	__mulsf3

__mulsf3:
	lea	%sp@(4),%a0
	moveml	%d2-%d5,%sp@-
	moveml	%a0@,%d4/%d5	| %d4 = v, %d5 = u
	subw	#8,%sp		| multiplication accumulator

	movel	%d5,%d0		| %d0 = u.exp
	swap	%d0
	movew	%d0,%d2		| %d2 = u.sign
	lsrw	#7,%d0
	andw	#0xff,%d0	| kill sign bit

	movel	%d4,%d1		| %d1 = v.exp
	swap	%d1
	eorw	%d1,%d2		| %d2 = u.sign ^ v.sign (in bit 31)
	lsrw	#7,%d1
	andw	#0xff,%d1	| kill sign bit

	andl	#0x7fffff,%d5	| remove exponent from u.mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_00
	orl	#0x800000,%d5	| restore implied leading "1"
	bra	L_10
L_00:	addw	#1,%d0		| "normalize" exponent
L_10:	tstl	%d5
	beq	retz		| multiplying zero

	andl	#0x7fffff,%d4	| remove exponent from v.mantissa
	tstw	%d1		| check for zero exponent - no leading "1"
	beq	L_01
	orl	#0x800000,%d4	| restore implied leading "1"
	bra	L_11
L_01:	addw	#1,%d1		| "normalize" exponent
L_11:	tstl	%d4
	beq	retz		| multiply by zero

	addw	%d1,%d0		| add exponents,
	subw	#BIAS4+16-8,%d0	| remove excess bias, acnt for repositioning

	clrl	%sp@		| initialize 64-bit product to zero
	clrl	%sp@(4)

| see Knuth, Seminumerical Algorithms, section 4.3. algorithm M

	movew	%d4,%d3
	mulu	%d5,%d3		| mulitply with bigit from multiplier
	movel	%d3,%sp@(4)	| store into result

	movel	%d4,%d3
	swap	%d3
	mulu	%d5,%d3
	addl	%d3,%sp@(2)	| add to result

	swap	%d5		| [TOP 8 BITS SHOULD BE ZERO !]

	movew	%d4,%d3
	mulu	%d5,%d3		| mulitply with bigit from multiplier
	addl	%d3,%sp@(2)	| store into result (no carry can occur here)

	movel	%d4,%d3
	swap	%d3
	mulu	%d5,%d3
	addl	%d3,%sp@		| add to result
				| [TOP 16 BITS SHOULD BE ZERO !]
	moveml	%sp@(2),%d4-%d5	| get the 48 valid mantissa bits
	clrw	%d5		| (pad to 64)
L_2:
	cmpl	#0x0000ffff,%d4	| multiply (shift) until
	bhi	L_3		|  1 in upper 16 result bits
	cmpw	#9,%d0		| give up for denormalized numbers
	ble	L_3
	swap	%d4		| (we're getting here only when multiplying
	swap	%d5		|  with a denormalized number; there's an
	movew	%d5,%d4		|  eventual loss of 4 bits in the rounding
	clrw	%d5		|  byte -- what a pity 8-)
	subw	#16,%d0		| decrement exponent
	bra	L_2
L_3:
	movel	%d5,%d1		| get rounding bits
	roll	#8,%d1
	movel	%d1,%d3		| see if sticky bit should be set
	andl	#0xffffff00,%d3
	beq	L_4
	orb	#1,%d1		| set "sticky bit" if any low-order set
L_4:	addw	#8,%sp		| remove accumulator from stack
	jmp	norm_sf		| (result in %d4)

retz:	clrl	%d0		| save zero as result
	addw	#8,%sp
	moveml	%sp@+,%d2-%d5
	rts			| no normalizing neccessary
