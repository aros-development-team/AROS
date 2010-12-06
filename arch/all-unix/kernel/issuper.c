#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(int, KrnIsSuper,
	  struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    return KernelBase->kb_PlatformData->supervisor;

    AROS_LIBFUNC_EXIT
}
