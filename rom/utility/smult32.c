/*
    $Id$
    $Log$
    Revision 1.1  1996/08/31 12:58:13  aros
    Merged in/modified for FreeBSD.

    Desc:
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
        #include <clib/utility_protos.h>

        __AROS_LH2(LONG, SMult32,

/*  SYNOPSIS */
        __AROS_LHA(long, arg1, D0),
        __AROS_LHA(long, arg2, D1),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 23, Utility)

/*  FUNCTION
        Performs the signed 32-bit multiplication of arg1 * arg2 and
        returns a signed 32 bit value.

    INPUTS
        arg1, arg2  -   32 bit signed longs

    RESULT
        arg1 * arg2

    NOTES
        This can perform the multiplication either using the machines
        native instructions (if they exist), or in software using a
        simple algorithm based on expanding algebraic products.

    EXAMPLE

        long a = 352543;
        long b = -52464;
        long c = SMult32(a,b);
        c == -1315946768

    BUGS

    SEE ALSO
        utility/UMult32(), utility/UMult64(), utility/SMult64()

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
    __AROS_FUNC_INIT

#ifdef HAS_32BITMULS
    return arg1 * arg2;
#else

    UWORD a1, b1, a0, b0;
    BOOL neg;

    /* Fix everything up so that -ve signs don't vanish */
    if(arg1 < 0)
    {
        neg = 1; arg1 = -arg1;
    }
    else
        neg = 0;

    if(arg2 <= 0)
    {
        neg ^= 1; arg2 = -arg2;
    }

    a1 = (arg1 >> 16) & 0xffff;
    a0 = arg1 & 0xffff;
    b1 = (arg2 >> 16) & 0xffff;
    b0 = arg2 & 0xffff;

    arg1 = (((a0 * b1) + (a1 * b0)) << 16) + (b0 * a0);
    return (neg ? -arg1 : arg1);
#endif

    __AROS_FUNC_EXIT
} /* SMult32 */
