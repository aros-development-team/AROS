#include <aros/libcall.h>

#include <kernel_base.h>

#include <signal.h>
#include <unistd.h>

AROS_LH0I(void, KrnCause,
	  struct KernelBase *, KernelBase, 3, Kernel)
{
    AROS_LIBFUNC_INIT

    raise(SIGUSR2);

    AROS_LIBFUNC_EXIT
}
