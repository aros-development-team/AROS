/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(LONG, SPCmp,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D1),
        AROS_LHA(float, fnum2, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 7, Mathffp)

/*  FUNCTION
        Compares two ffp numbers

    INPUTS

    RESULT
       +1 : fnum1 > fnum2
        0 : fnum1 = fnum2
       -1 : fnum1 < fnum2


        Flags:
          zero     : fnum2 = fnum1
          negative : fnum2 < fnum1
          overflow : 0

    BUGS

    INTERNALS
        ALGORITHM:
        1st case:
          fnum1 is negative and fnum2 is positive
            or
          ( exponent(fnum1) < exponent(fnum2) and signs are equal )
          -> fnum1 < fnum2

        2nd case:
          fnum1 is positive and fnum2 is negative
            or
          ( exponent(fnum1) > exponent(fnum2) and signs are equal )
          -> fnum2 > fnum1

        now the signs and exponents must be equal

        3rd case:
          fnum1 == fnum2

        4th case:
          mantisse(fnum1) < mantisse(fnum2)
          -> fnum1 < fnum2

        final case:
          fnum1 > fnum2

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(kprintf("SPCmp(%08x,%08x)=", fnum1, fnum2));

    /* fnum1 is negative and fnum2 is positive
    **  or
    ** exponent of fnum1 is less than the exponent of fnum2
    ** => fnum1 < fnum2
    */
    if ( (char)fnum1 < (char)fnum2 )
    {
        D(kprintf("-1\n"));
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return -1;
    }

    /* fnum1 is positive and fnum2 is negative
    **  or
    ** exponent of fnum1 is greater tban the exponent if fnum2
    ** => fnum1 > fnum2
    */
    if ((char) fnum1 > (char) fnum2 )
    {
        D(kprintf("1\n"));
        SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
        return 1;
    }

    /*the signs and exponents of fnum1 and fnum2 must now be equal
    **fnum1 == fnum2
    */
    if (fnum1 == fnum2)
    {
        D(kprintf("0\n"));
        SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return 0;
    }

    /* mantisse(fnum1) < mantisse(fnum2) */
    if (fnum1 < fnum2)
    {
        D(kprintf("-1\n"));
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return -1;
    }
    
    /* Mantisse(fnum1) > mantisse(fnum2) */
    D(kprintf("1\n"));
    SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 1;

    AROS_LIBFUNC_EXIT
}
