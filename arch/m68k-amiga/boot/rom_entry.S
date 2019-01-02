
Version = 46
Revision= 12

	.text

	.globl software_reset
	.globl _start
	.align 4 

rom_start:
	.word	0x1114
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
    .asciz "Copyright \251 1995-2019 "
    .asciz "The AROS Development Team, "
    .asciz "All Rights Reserved."
    .asciz ADATE

    /* Magic reset vector, used by some expansion ROMs and
     * softkick programs */
    .align 4
    .org   0xd0
    .global rom_magic_reset
rom_magic_reset:
    reset
    jmp rom_init
