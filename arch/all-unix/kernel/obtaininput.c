#include <aros/kernel.h>
#include <aros/libcall.h>

#include <fcntl.h>
#include <unistd.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0I(int, KrnObtainInput,
	  struct KernelBase *, KernelBase, 33, Kernel)
{
    AROS_LIBFUNC_INIT

    int res;

    /* Set our STDERR to non-blocking mode for RawMayGetChar() to work */
    res = KernelIFace.fcntl(STDERR_FILENO, F_GETFL);
    AROS_HOST_BARRIER
    res = KernelIFace.fcntl(STDERR_FILENO, F_SETFL, res|O_NONBLOCK);
    AROS_HOST_BARRIER

    return (res == -1) ? 0 : 1;

    AROS_LIBFUNC_EXIT
}
