#include <proto/exec.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include <kernel_debug.h>
#include "kernel_intern.h"

extern struct HostInterface *HostIFace;
int __vcformat (void * data, int (*outc)(int, void *),
		const char * format, va_list args);

AROS_LH2(int, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 12, Kernel)
{
    AROS_LIBFUNC_INIT

    int ret;

#if defined(HOST_OS_darwin) && defined(__ppc__)
    ret = __vcformat(KernelBase, (int (*)(int, void *))krnPutC, format, args);
#else
    ret = HostIFace->VKPrintF(format, args);
#endif
    AROS_HOST_BARRIER

    return ret;

    AROS_LIBFUNC_EXIT
}
