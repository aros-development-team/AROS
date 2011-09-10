	.text
	.align  16
	.globl  Kernel_10_KrnSti
	.type   Kernel_10_KrnSti, @function

Kernel_10_KrnSti:
	sti
	ret

	.size  Kernel_10_KrnSti, .-Kernel_10_KrnSti
