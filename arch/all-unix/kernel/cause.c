#include <aros/libcall.h>

#include <signal.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0I(void, KrnCause,
	  struct KernelBase *, KernelBase, 3, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelIFace.raise(SIGUSR2);

    AROS_LIBFUNC_EXIT
}
