/*
    $Id$
    $Log$
    Revision 1.4  1996/12/10 14:00:15  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.3  1996/10/24 22:51:46  aros
    Use proper Amiga datatypes (eg: ULONG not unsigned long)

    Revision 1.2  1996/10/24 15:51:38  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/31 12:58:13  aros
    Merged in/modified for FreeBSD.

    Desc: Signed 64 bit multiplication function.
    Lang: english
*/
#include "utility_intern.h"

/*****************************************************************************

    NAME */
#include <clib/utility_protos.h>

        AROS_LH2(QUAD, SMult64,

/*  SYNOPSIS */
        AROS_LHA(LONG, arg1, D0),
        AROS_LHA(LONG, arg2, D1),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 33, Utility)

/*  FUNCTION
        Compute the signed 64-bit product of arg1 * arg2.

    INPUTS
        arg1, arg2  -   32 bit signed numbers.

    RESULT
        arg1 * arg2

    NOTES

    EXAMPLE

    BUGS
        There is a problem under the current system in that it is very
        hard to return a 64-bit value.

    SEE ALSO
        utility/SMult32(), utility/UMult32(), utility/UMult64()

    INTERNALS
        This is essentially SMult32(), but without the code to calculate
        the product of the high 32 bits of the multiplicands.

        In fact all that is added is the 2^32 * ac term (see docs for
            SMult32().

    HISTORY
        29-10-95    digulla automatically created from
                            utility_lib.fd and clib/utility_protos.h
        18-08-96    iaint   Modified SMult32().

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

#ifdef HAS_64BITMULS
    return arg1 * arg2;
#else

    QUAD product;
    WORD a0, a1, b0, b1;
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

    a0 = arg1 & 0xFFFF;
    a1 = (arg1 >> 16) & 0xFFFF;

    b0 = arg2 & 0xFFFF;
    b1 = (arg2 >> 16) & 0xFFFF;

    /* In case numbers are small */
    if(a1 && b1)
        product = (QUAD)(a1 * b1) << 32;
    else
        product = 0;

    product += (((a1 * b0) + (a0 * b1)) << 16) + (a0 * b0);

    return (neg ? -product : product);
#endif

    AROS_LIBFUNC_EXIT
} /* SMult64 */
