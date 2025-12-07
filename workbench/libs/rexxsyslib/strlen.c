#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, Strlen,

/*  SYNOPSIS */
        AROS_LHA(UBYTE *, string, A0),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 48, RexxSys)

/*  FUNCTION
        This function returns the length of the provided ASCII string,
        and sets the Z flag of the m68k CCR if the string is empty.

    INPUTS
        string (A0) - A pointer to the first character of the ASCII string.

    RESULT
        length (D0) - The length of the ASCII string.
        The CCR Z flag is set if the length is zero.

    NOTES
        This function is excluded from the C headers and is only
        implemented for m68k CPUs.
        Registers A0, A2-A7, and D2-D7 are preserved across the call.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}
