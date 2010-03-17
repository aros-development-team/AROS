| double floating point negation routine
|
| written by Kai-Uwe Bloem (I5110401@dbstu1.bitnet).
| Based on a 80x86 floating point packet from comp.os.minix, written by P.Housel
|
|
| Revision 1.1, kub 12-89 :
| Ported over to 68k assembler
|
| Revision 1.0:
| original 8088 code from P.S.Housel

	.text
	.even
	.globl	__negdf2

__negdf2:			| floating point negate
	movel	%sp@(4),%d0	| do not negate if operand is 0.0
	orl	%sp@(8),%d0
	moveml	%sp@(4),%d0-%d1	| get number
	beq	L_1
	eorl	#0x80000000,%d0	| flip sign bit
L_1:	rts
