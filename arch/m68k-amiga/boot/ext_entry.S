Version = 46
Revision= 11

	.text

	.globl ext_entry
	.align 4 

	.word	0x1114
ext_entry:
	jmp software_reset

    // make programs that peek rom header happy
    .word 0x0000
    .word 0xffff
    .word Version
    .word Revision
    .word Version
    .word Revision
    .word 0xffff
    .word 0xffff
    .asciz "AROS EXT Extension Libraries"
    .asciz "Copyright \251 1995-2015 "
    .asciz "The AROS Development Team, "
    .asciz "All Rights Reserved."
    .asciz ADATE

    .align 4
    .org   0xd0
