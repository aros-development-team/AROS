| unsigned long division and modulus routines
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
|
|
| Revision 1.1, kub 03-90
| first version, replaces the appropriate routine from fixnum.s.
| Should be faster in more common cases. Division is done by 68000 divu
| operations if divisor is only 16 bits wide. Otherwise the normal division
| algorithm as described in various papers takes place. The division routine
| delivers the quotient in %d0 and the remainder in %d1, thus the implementation
| of the modulo operation is trivial.

	.text
	.even
	.globl	__udivsi3
	.globl	__umodsi3

__udivsi3:
	movel	%d2,%a0		| save registers
	movel	%d3,%a1
	clrl	%d0		| prepare result
	movel	%sp@(8),%d2	| get divisor
	beq	L_9		| divisor = 0 causes a division trap
	movel	%sp@(4),%d1	| get dividend
|== case 1) divident < divisor
	cmpl	%d2,%d1		| is divident smaller then divisor ?
	bcs	L_8		| yes, return immediately
|== case 2) divisor has <= 16 significant bits
	tstw	%sp@(8)
	bne	L_2		| divisor has only 16 bits
	movew	%d1,%d3		| save dividend
	clrw	%d1		| divide dvd.h by dvs
	swap	%d1
	beq	L_00		| (no division necessary if dividend zero)
	divu	%d2,%d1
L_00:	movew	%d1,%d0		| save quotient.h
	swap	%d0
	movew	%d3,%d1		| (%d1.h = remainder of prev divu)
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
	bcs	L_01
	subl	%d2,%d0		| big enough, subtract
	addw	#1,%d1		| and note bit in result
L_01:
	dbra	%d3,L_3
	exg	%d0,%d1		| put quotient and remainder in their registers
L_8:
	movel	%a1,%d3
	movel	%a0,%d2
	rts
L_9:
	divu	%d2,%d1		| cause division trap
	bra	L_8		| back to user


__umodsi3:
	movel	%sp@(8),%sp@-	| push divisor
	movel	%sp@(8),%sp@-	| push dividend
	jbsr	__udivsi3
	addql	#8,%sp
	movel	%d1,%d0		| return the remainder in %d0
	rts
