#include <proto/exec.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include "kernel_base.h"

AROS_LH2I(int, KrnBug,
         AROS_LHA(char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    int res = vfprintf(stderr, format, args);

#if !(defined(__linux__) || defined(__FreeBSD__))
    fsync (STDERR_FILENO);
#endif

    return res;

    AROS_LIBFUNC_EXIT
}
