#include <aros/ppc/asm.h>

#include "kernel_syscall.h"

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnSti, Kernel, 10)
	.type   AROS_SLIB_ENTRY(KrnSti, Kernel, 10), @function

AROS_SLIB_ENTRY(KrnSti, Kernel, 10):
	li	%r3, SC_STI
	blr

	.size  AROS_SLIB_ENTRY(KrnSti, Kernel, 10), .-AROS_SLIB_ENTRY(KrnSti, Kernel, 10)
