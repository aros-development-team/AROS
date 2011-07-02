
Version = 46
Revision= 10

	.text

	.globl software_reset
	.globl _start
	.align 4 

rom_start:
	.word	0x1111
_start:
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
    .asciz "AROS ROM Operating System and Libraries"
    .asciz "Copyright \251 2011 "
    .asciz "The AROS Development Team, "
    .asciz "All Rights Reserved."
    .asciz ADATE


    .align 4
    .org   0xd0
