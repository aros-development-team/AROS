#include <aros/x86_64/asm.h>

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnCli, Kernel, 9)
	.type   AROS_SLIB_ENTRY(KrnCli, Kernel, 9), @function

AROS_SLIB_ENTRY(KrnCli, Kernel, 9):
	cli
	ret

	.size  AROS_SLIB_ENTRY(KrnCli, Kernel, 9), .-AROS_SLIB_ENTRY(KrnCli, Kernel, 9)
