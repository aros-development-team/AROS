#include <aros/libcall.h>
#include <signal.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0I(int, KrnIsSuper,
	  struct KernelBase *, KernelBase, 13, Kernel)
{
    AROS_LIBFUNC_INIT

    return supervisor ? 1 : 0;

    AROS_LIBFUNC_EXIT
}
