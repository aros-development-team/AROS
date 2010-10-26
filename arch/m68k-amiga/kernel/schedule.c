#include <aros/kernel.h>

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <kernel_base.h>
#include <kernel_scheduler.h>

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
    BOOL wants_switch;

    wants_switch = core_Schedule();
    if (wants_switch)
    	    KrnSwitch();

    AROS_LIBFUNC_EXIT
}
