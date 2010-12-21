#include <aros/kernel.h>
#include <exec/execbase.h>
#include <proto/exec.h>

/*
 * This function is overriden here because it needs
 * SIGALRM definition which comes from host's signal.h,
 * and we don't want generic code to depend on host
 * includes in any way.
 */
#include <signal.h>

#include "kernel_base.h"

AROS_LH1(IPTR, KrnGetSystemAttr,
	 AROS_LHA(ULONG, id, D0),
	 struct KernelBase *, KernelBase, 29, Kernel)

{
    AROS_LIBFUNC_INIT

    switch (id)
    {
    case KATTR_Architecture:
	return (IPTR)AROS_ARCHITECTURE;

    case KATTR_VBlankEnable:
	return KernelBase->kb_VBlankEnable;

    case KATTR_TimerIRQ:
	return SIGALRM;

    default:
	return -1;
    }

    AROS_LIBFUNC_EXIT
}
