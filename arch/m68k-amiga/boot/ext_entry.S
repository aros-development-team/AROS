	.text

	.globl ext_entry
	.align 4 

	nop
ext_entry:
	jmp software_reset

	.align 8 
ROM_CheckSum:
	nop
	nop
