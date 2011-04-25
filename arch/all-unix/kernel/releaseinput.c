#include <aros/kernel.h>
#include <aros/libcall.h>

#include <fcntl.h>
#include <unistd.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0I(void, KrnReleaseInput,
	  struct KernelBase *, KernelBase, 34, Kernel)
{
    AROS_LIBFUNC_INIT

    int res;

    /* Reset nonblocking mode on STDERR */
    res = KernelIFace.fcntl(STDERR_FILENO, F_GETFL);
    AROS_HOST_BARRIER
    KernelIFace.fcntl(STDERR_FILENO, F_SETFL, res & ~O_NONBLOCK);
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
}
