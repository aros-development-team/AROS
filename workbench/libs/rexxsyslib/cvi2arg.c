#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(UBYTE *, CVi2arg,

/*  SYNOPSIS */
        AROS_LHA(LONG,  value,  D0),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 52, RexxSys)

/*  FUNCTION
        This function creates an argstring containing a string representation
        of the signed 32-bit integer value given.
        The argstring can be freed with DeleteArgstring().

    INPUTS
        value  (D0) - A signed 32-bit integer to generate the characters from.

    RESULT
        pointer (D0, A0) - A pointer on the same format that CreateArgstring() returns.

    NOTES
        This function is excluded from the C headers and is only
        implemented for m68k CPUs.
        The ARexx User's Reference Manual (1987) is incorrect.
        There is no "digits" argument.
        Registers A2-A7 and D2-D7 are preserved across the call.

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
