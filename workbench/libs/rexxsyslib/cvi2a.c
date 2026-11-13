#include "rexxsyslib_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, CVi2a,

/*  SYNOPSIS */
        AROS_LHA(UBYTE *, buffer, A0),
        AROS_LHA(LONG,    value,  D0),
        AROS_LHA(UWORD,   digits, D1),

/*  LOCATION */
        struct RxsLib *, RexxSysBase, 51, RexxSys)

/*  FUNCTION
        This function writes the digits of the signed 32-bit integer value
        to the buffer given, including the terminating NUL character.
        The function returns both the number of bytes written
        and a pointer to the NUL character in the buffer.

    INPUTS
        buffer (A0) - A pointer to the destination buffer.
        value  (D0) - A signed 32-bit integer to generate the characters from.
        digits (D1) - The max number of digits to write, including any sign
                      but excluding the terminating NUL character.
                      Zero means no limit.

    RESULT
        length  (D0) - The number of bytes written, excluding the terminating
                       NUL character.
        pointer (A0) - A pointer to the terminating NUL character.

    NOTES
        If digits is less than the number of characters needed, the least
        significant digits that fit will be written. Any sign needed will be
        included.
        This function is excluded from the C headers and is only
        implemented for m68k CPUs.
        Registers A2-A7 and D2-D7 are preserved across the call.

    EXAMPLE
        Setting value=-2000 and digits=3 will yield the string "-00".

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}
