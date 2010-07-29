#include <aros/libcall.h>

#include <stdarg.h>

#include "kernel_base.h"

/*****************************************************************************

    NAME */
#include <proto/kernel.h>

AROS_LH2(int, KrnBug,

/*  SYNOPSIS */
	AROS_LHA(const char *, format, A0),
        AROS_LHA(va_list, args, A1),

/*  LOCATION */
        struct KernelBase *, KernelBase, 12, Kernel)

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

    return 0;

    AROS_LIBFUNC_EXIT
}
