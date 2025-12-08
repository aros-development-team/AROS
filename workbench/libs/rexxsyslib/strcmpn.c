#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, StrcmpN,

/*  SYNOPSIS */
        AROS_LHA(UBYTE *, string1, A0),
        AROS_LHA(UBYTE *, string2, A1),
        AROS_LHA(ULONG, length, D0),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 42, RexxSys)

/*  FUNCTION
        This function compares the strings for the specified number of
        characters.

    INPUTS
        string1 (A0) - A pointer to the first character of the first string.
        string2 (A1) - Likewise for the second string.
        length  (D0) - The number of characters to compare.

    RESULT
        test (D0) - -1 if the second string is less, 0 if equal, 1 if greater.
        If one string ends it is considered less than a string that continues.
        The CCR flags are updated according to the result.

    NOTES
        This function is excluded from the C headers and is only
        implemented for m68k CPUs.
        The ARexx User's Reference Manual (1987) incorrectly says that 1 is
        returned if the first string is greater.
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
