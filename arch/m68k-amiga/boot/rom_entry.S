
Version = 46
Revision= 10

	.text

	.globl software_reset
	.align 4 

rom_start:
	.word	0x1111
software_reset:
	jmp rom_init

    // make programs that peek rom header happy

    .word 0x0000
    .word 0xffff
    .word Version
    .word Revision
    .word Version
    .word Revision
    .word 0xffff
    .word 0xffff
    .asciz "AROS ROM"

    .align 4
    .org   0xd0
