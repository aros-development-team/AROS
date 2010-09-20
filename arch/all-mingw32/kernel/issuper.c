#include <aros/libcall.h>

#include "kernel_base.h"

AROS_LH0I(int, KrnIsSuper,
	  struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    return *KernelIFace.SuperState ? 1 : 0;

    AROS_LIBFUNC_EXIT
}
