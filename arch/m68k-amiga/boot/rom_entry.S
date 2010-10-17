	.text

	.globl software_reset
	.align 4 

	.word	0x1111
software_reset:
	jmp rom_init

	.align 8 
ROM_CheckSum:
	nop
	nop
