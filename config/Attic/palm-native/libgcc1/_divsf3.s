| single floating point divide routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
|
| Revision 1.2, kub 01-90 :
| added support for denormalized numbers
|
| Revision 1.1, kub 12-89 :
| Created single float version for 68000
|
| Revision 1.0:
| original 8088 code from P.S.Housel for double floats

BIAS4	=	0x7F-1

	.text
	.even
	.globl	__divsf3
	.globl	_infinitysf

__divsf3:
	lea	%sp@(4),%a0	| pointer to parameters u and v
	moveml	%d2-%d5,%sp@-	| save registers
	moveml	%a0@,%d4/%d5	| %d4 = u, %d5 = v

	movel	%d4,%d0		| %d0 = u.exp
	swap	%d0
	movew	%d0,%d2		| %d2 = u.sign
	lsrw	#7,%d0
	andw	#0xff,%d0	| kill sign bit

	movel	%d5,%d1		| %d1 = v.exp
	swap	%d1
	eorw	%d1,%d2		| %d2 = u.sign ^ v.sign (in bit 31)
	lsrw	#7,%d1
	andw	#0xff,%d1	| kill sign bit

	andl	#0x7fffff,%d4	| remove exponent from u.mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_00
	orl	#0x800000,%d4	| restore implied leading "1"
	bra	L_10
L_00:	addw	#1,%d0		| "normalize" exponent
L_10:	tstl	%d4
	beq	retz		| dividing zero

	andl	#0x7fffff,%d5	| remove exponent from v.mantissa
	tstw	%d1		| check for zero exponent - no leading "1"
	beq	L_01
	orl	#0x800000,%d5	| restore implied leading "1"
	bra	L_11
L_01:	addw	#1,%d1		| "normalize" exponent
L_11:	tstl	%d5
	beq	divz		| divide by zero

	subw	%d1,%d0		| subtract exponents,
	addw	#BIAS4-8+1,%d0	|  add bias back in, account for shift
	addw	#34,%d0		|  add loop offset, +2 for extra rounding bits
				|   for denormalized numbers (2 implied by dbra)
	movew	#27,%d1		| bit number for "implied" pos (+4 for rounding)
	movel	#-1,%d3		|  zero quotient (for speed a one's complement)
	subl	%d5,%d4		| initial subtraction, u = u - v
L_2:
	btst	%d1,%d3		| divide until 1 in implied position
	beq	L_5

	addl	%d4,%d4
	bcs	L_4		| if carry is set, add, else subtract

	addxl	%d3,%d3		| shift quotient and set bit zero
	subl	%d5,%d4		| subtract, u = u - v
	dbra	%d0,L_2		| give up if result is denormalized
	bra	L_5
L_4:
	addxl	%d3,%d3		| shift quotient and clear bit zero
	addl	%d5,%d4		| add (restore), u = u + v
	dbra	%d0,L_2		| give up if result is denormalized
L_5:	subw	#2,%d0		| remove rounding offset for denormalized nums
	notl	%d3		| invert quotient to get it right

	movel	%d3,%d4		| save quotient mantissa
	clrw	%d1		| zero rounding bits
	jmp	norm_sf		| (registers on stack removed by norm_sf)

retz:	clrl	%d0		| zero destination
	moveml	%sp@+,%d2-%d5
	rts			| no normalization needed

divz:	movel	_infinitysf,%d0	| return infinty value
	moveml	%sp@+,%d2-%d5	| should really cause trap ?!?
	rts
