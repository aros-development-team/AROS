#include <aros/x86_64/asm.h>

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnCli, Kernel)
	.type   AROS_SLIB_ENTRY(KrnCli, Kernel), @function

AROS_SLIB_ENTRY(KrnCli, Kernel):
	cli
	ret

	.size  AROS_SLIB_ENTRY(KrnCli, Kernel), .-AROS_SLIB_ENTRY(KrnCli, Kernel)
