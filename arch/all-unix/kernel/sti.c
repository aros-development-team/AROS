#include <aros/libcall.h>

#include <signal.h>
#include <stdlib.h>

#include "kernel_base.h"
#include "kernel_intern.h"

AROS_LH0(void, KrnSti,
	  struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    if (!PD(KernelBase).supervisor)
    	sigprocmask(SIG_UNBLOCK, &PD(KernelBase).sig_int_mask, NULL);

    AROS_LIBFUNC_EXIT
}
