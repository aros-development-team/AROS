#include <aros/x86_64/asm.h>

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnIsSuper, Kernel)
	.type   AROS_SLIB_ENTRY(KrnIsSuper, Kernel), @function

AROS_SLIB_ENTRY(KrnIsSuper, Kernel):
	mov	%cs, %ax
	andq	$0x03, %rax		// RAX will be nonzero in user mode
	xorq	$0x03, %rax		// Invert the result
	ret

	.size  AROS_SLIB_ENTRY(KrnIsSuper, Kernel), .-AROS_SLIB_ENTRY(KrnIsSuper, Kernel)
