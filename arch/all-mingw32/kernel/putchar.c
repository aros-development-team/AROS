#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_base.h"

AROS_LH1(void, KrnPutChar,
	 AROS_LHA(char, c, D0),
	 struct KernelBase *, KernelBase, 25, Kernel)
{
    AROS_LIBFUNC_INIT

    Forbid();
    KernelIFace.core_putc(c);
    Permit();

    AROS_LIBFUNC_EXIT
}
