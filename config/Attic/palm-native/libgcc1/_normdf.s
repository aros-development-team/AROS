| double floating point normalization routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
| Revision 1.6, kub 04-90 :
| more robust handling exponent and sign handling for 32 bit integers. There
| are now overflow tests for 32 bit exponents, and bit 31 of the sign flag
| is or ed to bit 15 for later checks (i.e. both bits 31 and 15 are now sign
| bits). Take care, the upper 16 bits of rounding info are ignored for 32 bit
| integers !
|
| Revision 1.5, ++jrb 03-90:
| change _normdf interface to expect ints instead of shorts. easier
| to interface to 32 bit int code. this file is now pre-processed,
| with ___MSHORT___ defined when ints are 16 bits.
|
| Revision 1.4, kub 03-90 :
| export ___normdf entry to C language. Rename the internal entry to a name
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
	.globl	_infinitydf
	.globl	__normdf
	.globl	norm_df

	| C entry, for procs dealing with the internal representation :
	| double _normdf(long long mant, int exp, int sign, int rbits);
__normdf:
	lea	%sp@(4),%a0	| parameter pointer
	moveml	%d2-%d7,%sp@-	| save working registers
	moveml	%a0@+,%d4-%d5	| get mantissa
#ifdef ___MSHORT___
	movew	%a0@+,%d0		| get exponent
	movew	%a0@+,%d2		| get sign
	movew	%a0@+,%d1		| rounding information
#else
	movel	%a0@+,%d0		| get exponent
	movel	%a0@+,%d2		| get sign
	bpl	L_00		| or bit 31 to bit 15 for later tests
	orw	#0x8000,%d2
L_00:	movel	%a0@+,%d1		| rounding information

	cmpl	#0x7fff,%d0	| test exponent
	bgt	oflow
	cmpl	#-0x8000,%d0
	blt	eretz
#endif
	| internal entry for floating point package, saves time
	| %d0=u.exp, %d2=u.sign, %d1=rounding bits, %d4/%d5=mantissa
	| registers %d2-%d7 must be saved on the stack !
norm_df:
	movel	%d4,%d3		| rounding and u.mant == 0 ?
	orl	%d5,%d3
	bne	L_10
	tstb	%d1
	beq	retz
L_10:
	movel	%d4,%d3
	andl	#0xfffff000,%d3	| fast shift, 16 bits ?
	bne	L_2
	cmpw	#9,%d0		| shift is going to far; do normal shift
	ble	L_2		|  (minimize shifts here : 10l = 16l + 6r)
	swap	%d4		| yes, swap register halfs
	swap	%d5
	movew	%d5,%d4
	moveb	%d1,%d5		| some doubt about this one !
	lslw	#8,%d5
	clrw	%d1
	subw	#16,%d0		| account for swap
	bra	L_10
L_2:
	clrb	%d2		| sticky byte
	movel	#0xffe00000,%d6
L_3:	tstw	%d0		| divide (shift)
	ble	L_01		|  denormalized number
	movel	%d4,%d3
	andl	%d6,%d3		|  or until no bits above 53
	beq	L_4
L_01:	addw	#1,%d0		| increment exponent
	lsrl	#1,%d4
	roxrl	#1,%d5
	orb	%d1,%d2		| set sticky
	roxrb	#1,%d1		| shift into rounding bits
	bra	L_3
L_4:
	andb	#1,%d2
	orb	%d2,%d1		| make least sig bit sticky
	movel	#0xfff00000,%d6
L_5:	movel	%d4,%d3		| multiply (shift) until
	andl	%d6,%d3		| one in implied position
	bne	L_6
	subw	#1,%d0		| decrement exponent
	beq	L_6		|  too small. store as denormalized number
	addb	%d1,%d1		| some doubt about this one *
	addxl	%d5,%d5
	addxl	%d4,%d4
	bra	L_5
L_6:
	tstb	%d1		| check rounding bits
	bge	L_8		| round down - no action neccessary
	negb	%d1
	bvc	L_7		| round up
	bclr	#0,%d5		| tie case - round to even
	bra	L_8
L_7:
	clrl	%d1		| zero rounding bits
	addl	#1,%d5
	addxl	%d1,%d4
	tstw	%d0
	bne	L_02		| renormalize if number was denormalized
	addw	#1,%d0		| correct exponent for denormalized numbers
	bra	L_2
L_02:	movel	%d4,%d3		| check for rounding overflow
	andl	#0xffe00000,%d3
	bne	L_2		| go back and renormalize
L_8:
	movel	%d4,%d3		| check if normalization caused an underflow
	orl	%d5,%d3
	beq	eretz
	cmpw	#0,%d0		| check for exponent overflow or underflow
	blt	eretz
	cmpw	#2047,%d0
	bge	oflow

	lslw	#4,%d0		| re-position exponent
	andw	#0x8000,%d2	| sign bit
	orw	%d2,%d0
	swap	%d0		| map to upper word
	clrw	%d0
	andl	#0x0fffff,%d4	| top mantissa bits
	orl	%d0,%d4		| insert exponent and sign
	movel	%d4,%d0
	movel	%d5,%d1
	moveml	%sp@+,%d2-%d7
	rts

|#ifdef WITH_ERRNO
|	.globl	_errno		| from <errno.h>
|ERANGE	=	34
|#endif

eretz:
|#ifdef WITH_ERRNO
|#ifdef ___MSHORT___
|	movew	#ERANGE,_errno	| set errno
|#else
|	movel	#ERANGE,_errno	| set errno
|#endif
|#endif
retz:
	clrl	%d0		| return zero value
	clrl	%d1
	moveml	%sp@+,%d2-%d7
	rts

oflow:
|#ifdef WITH_ERRNO
|#ifdef ___MSHORT___
|	movew	#ERANGE,_errno	| set errno
|#else
|	movel	#ERANGE,_errno	| set errno
|#endif
|#endif
	moveml	_infinitydf,%d0-%d1 | return infinty value
	andw	#0x8000,%d2	| get sign bit of argument
	swap	%d2
	clrw	%d2
	orl	%d2,%d0
	moveml	%sp@+,%d2-%d7	| should really cause trap ?!?
	rts

_infinitydf:			| +infinity as proposed by IEEE
	.long	0x7ff00000,0x00000000
