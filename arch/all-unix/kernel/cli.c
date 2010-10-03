#include <aros/libcall.h>

#include <stdlib.h>
#include <signal.h>

#include "kernel_base.h"
#include "kernel_intern.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH0(void, KrnCli,

/*  SYNOPSIS */

/*  LOCATION */
	struct KernelBase *, KernelBase, 9, Kernel)

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

    sigprocmask(SIG_BLOCK, &PD(KernelBase).sig_int_mask, NULL);

    AROS_LIBFUNC_EXIT
}
