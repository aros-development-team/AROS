#include <aros/ppc/asm.h>

#include "kernel_syscall.h"

	.text
	.align  16
	.globl  AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)
	.type   AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13), @function

AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13):
	li	%r3, SC_ISSUPERSTATE
	blr

	.size  AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13), .-AROS_SLIB_ENTRY(KrnIsSuper, Kernel, 13)
