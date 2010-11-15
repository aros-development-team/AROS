#include <aros/libcall.h>

#include <unistd.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH1I(void, KrnPutChar,
	 AROS_LHA(char, c, D0),
	 struct KernelBase *, KernelBase, 25, Kernel)
{
    AROS_LIBFUNC_INIT

    KernelIFace.write (STDERR_FILENO, &c, 1);
    AROS_HOST_BARRIER

    AROS_LIBFUNC_EXIT
}
