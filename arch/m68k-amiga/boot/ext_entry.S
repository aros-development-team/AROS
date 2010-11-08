	.text

	.globl ext_entry
	.align 4 

	nop
ext_entry:
	jmp 0xf80002

	.align 8 
ROM_CheckSum:
	nop
	nop
