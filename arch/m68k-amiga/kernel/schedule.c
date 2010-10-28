#include <aros/kernel.h>

#include <proto/exec.h>
#include <exec/execbase.h>
#include <kernel_base.h>
#include <kernel_syscall.h>

#include "kernel_scheduler.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnSchedule,

/*  SYNOPSIS */

/*  LOCATION */
         struct KernelBase *, KernelBase, 6, Kernel)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Task switching enabled? */
    if (SysBase->TDNestCnt >= 0)
    	    return;

    /* Anything need to be scheduled? */
    if (!(SysBase->AttnResched & ARF_AttnSwitch))
    	    return;

    if (core_Schedule())
    	    Switch();

    AROS_LIBFUNC_EXIT
}
