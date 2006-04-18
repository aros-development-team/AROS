| single float to double float conversion routine
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

BIAS4	=	0x7F-1
BIAS8	=	0x3FF-1

	.text
	.even
	.globl	__extendsfdf2

__extendsfdf2:
	lea	%sp@(4),%a0	| parameter pointer
	moveml	%d2-%d7,%sp@-	| save regs to keep norm_df happy
	movel	%a0@,%d4		| get number
	clrl	%d5		| prepare double mantissa

	movew	%a0@,%d0		| extract exponent
	movew	%d0,%d2		| extract sign
	lsrw	#7,%d0
	andw	#0xff,%d0	| kill sign bit (exponent is 8 bits)

	andl	#0x7fffff,%d4	| remove exponent from mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_0		| for denormalized numbers
	orl	#0x800000,%d4	| restore implied leading "1"
	bra	L_1
L_0:	addw	#1,%d0		| "normalize" exponent
L_1:
	addw	#BIAS8-BIAS4-3,%d0	| adjust bias, account for shift
	clrw	%d1		| dummy rounding info

	jmp	norm_df
