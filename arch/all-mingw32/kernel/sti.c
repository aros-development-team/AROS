#include <aros/libcall.h>

#include "kernel_base.h"

AROS_LH0I(void, KrnSti,
	  struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelIFace.core_intr_enable();

    AROS_LIBFUNC_EXIT
}
