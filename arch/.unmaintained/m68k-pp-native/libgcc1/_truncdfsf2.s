| double float to single float conversion routine
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
	.globl	__truncdfsf2

__truncdfsf2:
	lea	%sp@(4),%a0	| parameter pointer
	moveml	%d2-%d5,%sp@-	| save regs
	moveml	%a0@,%d4-%d5	| get number

	movew	%a0@,%d0		| extract exponent
	movew	%d0,%d2		| extract sign
	lsrw	#4,%d0
	andw	#0x7ff,%d0	| kill sign bit

	andl	#0x0fffff,%d4	| remove exponent from mantissa
	tstw	%d0		| check for zero exponent - no leading "1"
	beq	L_0		| for denormalized numbers
	orl	#0x100000,%d4	| restore implied leading "1"
	bra	L_1
L_0:	addw	#1,%d0		| "normalize" exponent
L_1:
	addw	#BIAS4-BIAS8,%d0	| adjust bias

	addl	%d5,%d5		| shift up to realign mantissa for floats
	addxl	%d4,%d4
	addl	%d5,%d5
	addxl	%d4,%d4
	addl	%d5,%d5
	addxl	%d4,%d4

	movel	%d5,%d1		| set rounding bits
	roll	#8,%d1
	andl	#0x00ffffff,%d5	| check to see if sticky bit should be set
	beq	L_2
	orb	#1,%d1		| set sticky bit
L_2:
	jmp	norm_sf		| (leave regs on stack for norm_sf)
