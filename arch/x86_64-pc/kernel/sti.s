#include <aros/x86_64/asm.h>

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnSti, Kernel)
	.type   AROS_SLIB_ENTRY(KrnSti, Kernel), @function

AROS_SLIB_ENTRY(KrnSti, Kernel):
	sti
	ret

	.size  AROS_SLIB_ENTRY(KrnSti, Kernel), .-AROS_SLIB_ENTRY(KrnSti, Kernel)
