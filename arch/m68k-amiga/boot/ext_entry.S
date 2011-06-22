Version = 46
Revision= 10

	.text

	.globl ext_entry
	.align 4 

	nop
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
    .asciz "Copyright \251 2011 "
    .asciz "The AROS Development Team, "
    .asciz "All Rights Reserved."
    .asciz ADATE

    .align 4
    .org   0xd0
