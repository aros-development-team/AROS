/*
    $Id$
    $Log$
    Revision 1.3  1996/10/24 22:51:47  aros
    Use proper Amiga datatypes (eg: ULONG not unsigned long)

    Revision 1.2  1996/10/24 15:51:39  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/31 12:58:13  aros
    Merged in/modified for FreeBSD.

    Desc:
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <clib/utility_protos.h>

        AROS_LH2(ULONG, UMult32,

/*  SYNOPSIS */
        AROS_LHA(ULONG        , arg1, D0),
        AROS_LHA(ULONG        , arg2, D1),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 24, Utility)

/*  FUNCTION
        Performs an unsigned 32-bit multiplication of arg1 * arg2 and
        returns a 32 bit value.

    INPUTS
        arg1, arg2  -   32 bit unsigned longs

    RESULT
        arg1 * arg2

    NOTES
        This can perform the multiplication either using the machines
        native instructions (if they exist), or in software using a
        simple algorithm (three multiplications, two shifts and
        an addition.

    EXAMPLE

        LONG a = 352543;
        LONG b = 52464;
        LONG c = UMult32(a,b);
        c == 1315946768

    BUGS

    SEE ALSO
        utility/SMult32(), utility/UMult64(), utility/SMult64()

    INTERNALS
        We are performing the operation:


            (2^16 * a + b) * (2^16 * c + d)
          = 2^32 * ab + 2^16 * ad + 2^16 * bc + bd
          = 2^32 * ab + 2^16 ( ad + bc ) + bd

        Now since the result is a 32-bit number, the 2^32 term will have
        no effect. (Since 2^32 > max (32-bit number).

        Therefore:
        product = 2^16( ad + bc ) + bd

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        18-08-96    iaint   Implemented as described above.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef HAS_32BITMULU
    return arg1 * arg2;
#else

    UWORD a0, a1, b0, b1;

    a1 = (arg1 >> 16) & 0xffff;
    a0 = arg1 & 0xffff;
    b1 = (arg2 >> 16) & 0xffff;
    b0 = arg2 & 0xffff;

    return (((a0 * b1) + (a1 * b0)) << 16) + (b0 * a0);
#endif

    AROS_LIBFUNC_EXIT
} /* UMult32 */
