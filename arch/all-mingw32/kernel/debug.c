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

/* Incomplete and not implemented */

AROS_LH3(void, KrnRegisterModule,
	 AROS_LHA(const char *, name, A0),
	 AROS_LHA(struct sheader *, sections, A1),
	 AROS_LHA(struct elfheader *, eh, A2),
	 struct KernelBase *, KernelBase, 21, Kernel)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, KrnUnregisterModule,
		AROS_LHA(void *,		address, A0),
		struct KernelBase *, KernelBase, 22, Kernel)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}
