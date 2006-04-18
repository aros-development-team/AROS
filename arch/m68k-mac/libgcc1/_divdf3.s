| double floating point divide routine
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
	.globl	__divdf3
	.globl	_infinitydf

__divdf3:
	lea	%sp@(4),%a0	| pointer to parameters u and v
	moveml	%d2-%d7,%sp@-	| save registers
	moveml	%a0@,%d4-%d5/%d6-%d7	| %d4-%d5 = u, %d6-%d7 = v

	movel	%d4,%d0		| %d0 = u.exp
	swap	%d0
	movew	%d0,%d2		| %d2 = u.sign
	lsrw	#4,%d0
	andw	#0x07ff,%d0	| kill sign bit

	movel	%d6,%d1		| %d1 = v.exp
	swap	%d1
	eorw	%d1,%d2		| %d2 = u.sign ^ v.sign (in bit 31)
	lsrw	#4,%d1
	andw	#0x07ff,%d1	| kill sign bit

	andl	#0x0fffff,%d4	| remove exponent from u.mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_00
	orl	#0x100000,%d4	| restore implied leading "1"
	bra	L_10
L_00:	addw	#1,%d0		| "normalize" exponent
L_10:	movel	%d4,%d3
	orl	%d5,%d3
	beq	retz		| dividing zero

	andl	#0x0fffff,%d6	| remove exponent from v.mantissa
	tstw	%d1		| check for zero exponent - no leading "1"
	beq	L_01
	orl	#0x100000,%d6	| restore implied leading "1"
	bra	L_11
L_01:	addw	#1,%d1		| "normalize" exponent
L_11:	movel	%d6,%d3
	orl	%d7,%d3
	beq	divz		| divide by zero

	movew	%d2,%a0		| save sign

	subw	%d1,%d0		| subtract exponents,
	addw	#BIAS8-11+1,%d0	|  add bias back in, account for shift
	addw	#66,%d0		|  add loop offset, +2 for extra rounding bits
				|   for denormalized numbers (2 implied by dbra)
	movew	#24,%d1		| bit number for "implied" pos (+4 for rounding)
	movel	#-1,%d2		| zero the quotient
	movel	#-1,%d3		|  (for speed it is a one's complement)
	subl	%d7,%d5		| initial subtraction,
	subxl	%d6,%d4		| u = u - v
L_2:
	btst	%d1,%d2		| divide until 1 in implied position
	beq	L_5

	addl	%d5,%d5
	addxl	%d4,%d4
	bcs	L_4		| if carry is set, add, else subtract

	addxl	%d3,%d3		| shift quotient and set bit zero
	addxl	%d2,%d2
	subl	%d7,%d5		| subtract
	subxl	%d6,%d4		| u = u - v
	dbra	%d0,L_2		| give up if result is denormalized
	bra	L_5
L_4:
	addxl	%d3,%d3		| shift quotient and clear bit zero
	addxl	%d2,%d2
	addl	%d7,%d5		| add (restore)
	addxl	%d6,%d4		| u = u + v
	dbra	%d0,L_2		| give up if result is denormalized
L_5:	subw	#2,%d0		| remove rounding offset for denormalized nums
	notl	%d2		| invert quotient to get it right
	notl	%d3

	movel	%d2,%d4		| save quotient mantissa
	movel	%d3,%d5
	movew	%a0,%d2		| get sign back
	clrw	%d1		| zero rounding bits
	jmp	norm_df		| (registers on stack removed by norm_df)

retz:	clrl	%d0		| zero destination
	clrl	%d1
	moveml	%sp@+,%d2-%d7
	rts			| no normalization needed

divz:	moveml	_infinitydf,%d0-%d1 | return infinty value
	moveml	%sp@+,%d2-%d7	| should really cause trap ?!?
	rts
