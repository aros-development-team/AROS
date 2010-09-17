#include <aros/libcall.h>

#include "kernel_base.h"

AROS_LH0I(void, KrnCli,
	  struct KernelBase *, KernelBase, 9, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelIFace.core_intr_disable();

    AROS_LIBFUNC_EXIT
}
