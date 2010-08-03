#include <proto/exec.h>

#include <stdarg.h>

#include "kernel_base.h"

AROS_LH2I(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    int res;

    Forbid();
    res = HostIFace->VKPrintF(format, args);
    Permit();

    return res;

    AROS_LIBFUNC_EXIT
}
