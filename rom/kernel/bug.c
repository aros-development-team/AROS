#include <aros/libcall.h>

#include <stdarg.h>
#include <stdio.h>

#include "kernel_base.h"
#include "kernel_debug.h"

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

    return __vcformat(NULL, krnPutC, format, args);

    AROS_LIBFUNC_EXIT
}
