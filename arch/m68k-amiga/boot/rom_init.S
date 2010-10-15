	.text

	.align 4 

	.globl rom_init
rom_init:
	moveq	#0x3, %d0
	move.b	%d0, 0xbfe201
	moveq	#0x2, %d0
	move.b	%d0, 0xbfe001

	/* TODO: Check the stack RAM
	 *       before we start the ROM proper.
	 */
	lea.l	_stack_end,%sp
	jra	start
