#include <aros/x86_64/asm.h>

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)
	.type   AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13), @function

AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13):
	mov	%cs, %ax
	andq	$0x03, %rax		// RAX will be nonzero in user mode
	xorq	$0x03, %rax		// Invert the result
	ret

	.size  AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13), .-AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)
