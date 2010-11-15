#include <proto/exec.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include "hostinterface.h"
#include "kernel_base.h"
#include "kernel_intern.h"

extern struct HostInterface *HostIFace;

AROS_LH2I(int, KrnBug,
         AROS_LHA(char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    int ret;

    ret = HostIFace->VKPrintF(format, args);
    AROS_HOST_BARRIER

    return ret;

    AROS_LIBFUNC_EXIT
}
