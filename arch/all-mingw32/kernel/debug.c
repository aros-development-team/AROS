#include <aros/kernel.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <stdarg.h>

#include "kernel_intern.h"

AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    int res;

    /* Windows console output aborts if task switch occurs while it's running */
    if (SysBase)
    	Forbid();
    res = HostIFace->VKPrintF(format, args);
    if (SysBase)
    	Permit();
    return res;
    
    AROS_LIBFUNC_EXIT
}
