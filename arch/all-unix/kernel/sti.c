#include <aros/libcall.h>

#include <signal.h>
#include <stdlib.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(void, KrnSti,
	  struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    if (!KernelBase->kb_PlatformData->supervisor)
    	sigprocmask(SIG_UNBLOCK, &KernelBase->kb_PlatformData->sig_int_mask, NULL);

    AROS_LIBFUNC_EXIT
}
