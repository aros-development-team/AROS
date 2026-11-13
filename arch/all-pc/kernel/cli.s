	.text
	.align  16
	.globl  Kernel_9_KrnCli
	.type   Kernel_9_KrnCli, @function

Kernel_9_KrnCli:
	cli
	ret

	.size  Kernel_9_KrnCli, .-Kernel_9_KrnCli
