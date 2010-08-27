#include <proto/exec.h>
#include <proto/kernel.h>

#include <unistd.h>

#include "kernel_base.h"

AROS_LH1(void, KrnPutChar,
	 AROS_LHA(char, c, D0),
	 struct KernelBase *, KernelBase, 25, Kernel)
{
    AROS_LIBFUNC_INIT

    write (STDERR_FILENO, &c, 1);
    /* Make sure it makes it to the user. Slow but save.
       On Linux this gives an error (stderr is already unbuffered) */
#if !(defined(__linux__) || defined(__FreeBSD__))
    fsync (STDERR_FILENO);
#endif

    AROS_LIBFUNC_EXIT
}
