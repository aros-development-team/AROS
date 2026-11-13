#include <aros/ppc/asm.h>

#include "kernel_syscall.h"

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnCli, Kernel, 9)
	.type   AROS_SLIB_ENTRY(KrnCli, Kernel, 9), @function

AROS_SLIB_ENTRY(KrnCli, Kernel, 9):
	li	%r3, SC_CLI
	sc
	blr

	.size  AROS_SLIB_ENTRY(KrnCli, Kernel, 9), .-AROS_SLIB_ENTRY(KrnCli, Kernel, 9)
