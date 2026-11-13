| single floating point normalization routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
|
| Revision 1.4, kub 03-90 :
| export ___normsf entry to C language. Rename the internal entry to a name
| not accessible from C to prevent crashes
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
	.globl	_infinitysf
	.globl	__normsf
	.globl	norm_sf

	| C entry, for procs dealing with the internal representation :
	| float _normsf(long mant, short exp, short sign, short rbits);
__normsf:
	lea	%sp@(4),%a0	| parameter pointer
	moveml	%d2-%d5,%sp@-	| save working registers
	movel	%a0@+,%d4		| get mantissa
	movew	%a0@+,%d0		| get exponent
	movew	%a0@+,%d2		| get sign
	movew	%a0@+,%d1		| rounding information

	| internal entry for floating point package, saves time
	| %d0=u.exp, %d2=u.sign, %d1=rounding bits, %d4/%d5=mantissa
	| registers %d2-%d7 must be saved on the stack !
norm_sf:
	tstl	%d4		| rounding and u.mant == 0 ?
	bne	L_00
	tstb	%d1
	beq	retz
L_00:
	clrb	%d2		| "sticky byte"
	movel	#0xff000000,%d5
L_10:	tstw	%d0		| divide (shift)
	ble	L_01		|  denormalized number
	movel	%d4,%d3
	andl	%d5,%d3		|  or until no bits above 23
	beq	L_2
L_01:	addw	#1,%d0		| increment exponent
	lsrl	#1,%d4
	orb	%d1,%d2		| set "sticky"
	roxrb	#1,%d1		| shift into rounding bits
	bra	L_10
L_2:
	andb	#1,%d2
	orb	%d2,%d1		| make least sig bit "sticky"
	movel	#0xff800000,%d5
L_3:	movel	%d4,%d3		| multiply (shift) until
	andl	%d5,%d3		| one in "implied" position
	bne	L_4
	subw	#1,%d0		| decrement exponent
	beq	L_4		|  too small. store as denormalized number
	addb	%d1,%d1		| some doubt about this one *
	addxl	%d4,%d4
	bra	L_3
L_4:
	tstb	%d1		| check rounding bits
	bge	L_6		| round down - no action neccessary
	negb	%d1
	bvc	L_5		| round up
	bclr	#0,%d4		| tie case - round to even
	bra	L_6
L_5:
	clrw	%d1		| zero rounding bits
	addl	#1,%d4
	tstw	%d0
	bne	L_02		| renormalize if number was denormalized
	addw	#1,%d0		| correct exponent for denormalized numbers
	bra	L_10
L_02:	movel	%d4,%d3		| check for rounding overflow
	andl	#0xff000000,%d3
	bne	L_10		| go back and renormalize
L_6:
	tstl	%d4		| check if normalization caused an underflow
	beq	retz
	cmpw	#0,%d0		| check for exponent overflow or underflow
	blt	retz
	cmpw	#255,%d0
	bge	oflow

	lslw	#7,%d0		| re-position exponent
	andw	#0x8000,%d2	| sign bit
	orw	%d2,%d0
	swap	%d0		| map to upper word
	clrw	%d0
	andl	#0x7fffff,%d4	| top mantissa bits
	orl	%d4,%d0		| insert exponent and sign
	moveml	%sp@+,%d2-%d5
	rts

retz:	clrl	%d0
	moveml	%sp@+,%d2-%d5
	rts

oflow:	movel	_infinitysf,%d0	| return infinty value
	moveml	%sp@+,%d2-%d5	| should really cause trap ?!?
	rts

_infinitysf:			| +infinity as proposed by IEEE
	.long	0x7f800000
