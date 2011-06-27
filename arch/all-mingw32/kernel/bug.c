#include <aros/libcall.h>
#include <exec/types.h>

#include <stdarg.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_mingw32.h"

AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 12, Kernel)
{
    AROS_LIBFUNC_INIT

    return myvkprintf(format, args);

    AROS_LIBFUNC_EXIT
}

/* internal function for kernel when KernelBase is NULL */
int __KrnBugBoot(const char *format, va_list args)
{
    return myvkprintf(format, args);
}
