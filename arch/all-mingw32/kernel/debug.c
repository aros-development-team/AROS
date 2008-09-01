#include <aros/kernel.h>
#include <aros/libcall.h>
#include <stdarg.h>

#include "kernel_intern.h"

AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    return HostIFace->VKPrintF(format, args);
    
    AROS_LIBFUNC_EXIT
}
