#include <aros/libcall.h>

#include "kernel_base.h"
#include "kernel_unix.h"

AROS_LH0I(int, KrnIsSuper,
	  struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    return SupervisorCount;

    AROS_LIBFUNC_EXIT
}
