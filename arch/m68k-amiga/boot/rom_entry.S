	.text

	.globl software_reset
	.align 4 

rom_start:
	.word	0x1111
software_reset:
	jmp rom_init

	.align 8 
	.globl ROM_CheckSum
ROM_CheckSum:
	nop
	nop
