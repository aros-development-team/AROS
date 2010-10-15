| long integer to double float conversion routine
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
	.globl	__floatsidf

__floatsidf:
	movel	%sp@(4),%d0	| get the 4-byte integer
	moveml	%d2-%d7,%sp@-	| save registers to make norm_df happy

	movel	%d0,%d4		| prepare result mantissa
	clrl	%d5
	movew	#BIAS8+32-11,%d0	| radix point after 32 bits
L_0:
	movel	%d4,%d2		| set sign flag
	swap	%d2
	tstw	%d2		| check sign of number
	bge	L_1		| nonnegative
	negl	%d4		| take absolute value
L_1:
	clrw	%d1		| set rounding = 0
	jmp	norm_df
