	.text
	.align  16
	.globl  Kernel_13_KrnIsSuper
	.type   Kernel_13_KrnIsSuper, @function

Kernel_13_KrnIsSuper:
	mov	%cs, %ax
	and	$0x03, %eax		// Lower two bits are CPL
	xor	$0x03, %eax		// 0x03 will give zero, 0x00 will give nonzero
	ret

	.size  Kernel_13_KrnIsSuper, .-Kernel_13_KrnIsSuper
