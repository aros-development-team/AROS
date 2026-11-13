#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, CVa2i,

/*  SYNOPSIS */
        AROS_LHA(UBYTE *, buffer, A0),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 50, RexxSys)

/*  FUNCTION
        This function converts an ASCII string to a signed 32-bit integer.
        The string scanning is stopped when a non-digit character is found or
        the integer value would overflow.
        The function returns both the number of bytes scanned
        and the integer value.

    INPUTS
        buffer (A0) - A pointer to the first character of the ASCII string.

    RESULT
        value  (D0) - The signed integer obtained from the string.
        digits (D1) - The number of digits scanned from the string, plus one
                      if there was a leading + or -.

    NOTES
        This function is excluded from the C headers and is only
        implemented for m68k CPUs.
        The ARexx User's Reference Manual (1987) is incorrect.
        The result registers listed above are the correct ones.
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
