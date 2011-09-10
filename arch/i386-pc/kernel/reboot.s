# One more important place. We come here upon warm restart.
# Here we set the stack pointer to some safe place and use core_Kick
# to run kernel_cstart() routine with our old boot taglist.

	.globl	core_Reboot
	.type	core_Reboot, @function
core_Reboot:
	cli
	cld
	movl	$0x1000, %esp
	movl	BootMsg, %eax
	pushl	$kernel_cstart
	pushl	%eax
	call	core_Kick
