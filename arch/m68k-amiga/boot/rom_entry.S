	.text

	.globl software_reset
	.align 4 

rom_start:
	.word	0x1111
software_reset:
	/* Needed to force an absolute jump */
	.word	0x4ef9
	.long	rom_cart

	.align 8 
ROM_CheckSum:
	nop
	nop

rom_cart:
	/* Check for cartidge ROM */
	move.l	0xf00000,%a0
	move.w	%a0@(0),%d0
	cmp.w	#0x1111,%d0
	bne.s	0f
	move.l	%a0@(4),%d0
	cmp.l	#rom_cart,%d0
	bne.s	0f
	move.l	#0x40000,%sp	/* Safe stack at Chip Mem 128K */
	jmp	%a0@(2)
0:
	jmp rom_init

