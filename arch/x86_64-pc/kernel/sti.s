#include <aros/x86_64/asm.h>

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnSti, Kernel, 10)
	.type   AROS_SLIB_ENTRY(KrnSti, Kernel, 10), @function

AROS_SLIB_ENTRY(KrnSti, Kernel, 10):
	sti
	ret

	.size  AROS_SLIB_ENTRY(KrnSti, Kernel, 10), .-AROS_SLIB_ENTRY(KrnSti, Kernel, 10)
