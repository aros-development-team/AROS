| long division and modulus routines
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
|
|
| Revision 1.1, kub 03-90
| first version, replaces the appropriate routine from fixnum.s.
| Should be faster in more common cases. Division is done by 68000 divu
| operations if divisor is only 16 bits wide. Otherwise the normal division
| algorithm as described in various papers takes place. The division routine
| delivers the quotient in d0 and the remainder in %d1, thus the implementation
| of the modulo operation is trivial. We gain some extra speed by inlining
| the division code here instead of calling _udivsi3.

	.text
	.even
	.globl	__divsi3
	.globl	__modsi3

__divsi3:
	movel	%d2,%a0		| save registers
	movel	%d3,%a1
	clrw	%sp@-		| sign flag
	clrl	%d0		| prepare result
	movel	%sp@(10),%d2	| get divisor
	beq	L_9		| divisor = 0 causes a division trap
	bpl	L_00		| divisor < 0 ?
	negl	%d2		| negate it
	notw	%sp@		| remember sign
L_00:	movel	%sp@(6),%d1	| get dividend
	bpl	L_01		| dividend < 0 ?
	negl	%d1		| negate it
	notw	%sp@		| remember sign
L_01:
|== case 1) divident < divisor
	cmpl	%d2,%d1		| is divident smaller then divisor ?
	bcs	L_8		| yes, return immediately
|== case 2) divisor has <= 16 significant bits
	tstw	%sp@(10)
	bne	L_2		| divisor has only 16 bits
	movew	%d1,%d3		| save dividend
	clrw	%d1		| divide dvd.h by dvs
	swap	%d1
	beq	L_02		| (no division necessary if dividend zero)
	divu	%d2,%d1
L_02:	movew	%d1,%d0		| save quotient.h
	swap	%d0
	movew	%d3,%d1		| (%d0.h = remainder of prev divu)
	divu	%d2,%d1		| divide dvd.l by dvs
	movew	%d1,%d0		| save quotient.l
	clrw	%d1		| get remainder
	swap	%d1
	bra	L_8		| and return
|== case 3) divisor > 16 bits (corollary is dividend > 16 bits, see case 1)
L_2:
	moveq	#31,%d3		| loop count
L_3:
	addl	%d1,%d1		| shift divident ...
	addxl	%d0,%d0		|  ... into %d0
	cmpl	%d2,%d0		| compare with divisor
	bcs	L_03
	subl	%d2,%d0		| big enough, subtract
	addw	#1,%d1		| and note bit into result
L_03:
	dbra	%d3,L_3
	exg	%d0,%d1		| put quotient and remainder in their registers
L_8:
	tstw	%sp@(6)		| must the remainder be corrected ?
	bpl	L_04
	negl	%d1		| yes, apply sign
| the following line would be correct if modulus is defined as in algebra
|	addl	%sp@(6),%d1	| algebraic correction: modulus can only be >= 0
L_04:	tstw	%sp@+		| result should be negative ?
	bpl	L_05
	negl	%d0		| yes, negate it
L_05:
	movel	%a1,%d3
	movel	%a0,%d2
	rts
L_9:
	divu	%d2,%d1		| cause division trap
	bra	L_8		| back to user


__modsi3:
	movel	%sp@(8),%sp@-	| push divisor
	movel	%sp@(8),%sp@-	| push dividend
	jbsr	__divsi3
	addql	#8,%sp
	movel	%d1,%d0		| return the remainder in %d0
	rts
