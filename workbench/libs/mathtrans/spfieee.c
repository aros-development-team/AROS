/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Convert single precision ieee number to FFP number

    RESULT
      Motorola fast floating point number

      flags:
         zero     : result is zero
         negative : result is negative
         overflow : exponent of the ieee-number was out of range for
                    ffp

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH1(float, SPFieee,
    AROS_LHA(float, ieeenum, D0),
    struct Library *, MathTransBase, 18, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    
    /* check for ieeenum == 0 */
    if (0 == ieeenum)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    /* calculate the exponent  */
    Res = (ieeenum & IEEESPExponent_Mask) >> 23;
    Res = Res - 126 + 0x40;
    
    /* exponent too big / small */
    if ((char) Res < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }
    
    Res |= (ieeenum << 8) | 0x80000000;
    
    /* check ieeenum-number for a sign */
    if (ieeenum < 0)
    {
        Res |= FFPSign_Mask;
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
