/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate the hyperbolic cosine of the IEEE single precision number

    RESULT
      IEEE single precision floating point number

      flags:
        zero     : result is zero
        negative : 0 (not possible)
        overflow : result too big for ffp-number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      <code>
      cosh(x) = (1/2)*( e^x + e^(-x) )

      cosh( |x| >= 9  ) = (1/2) * (e^x);
      </code>

    HISTORY
*/

AROS_LH1(float, IEEESPCosh,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 11, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    /* cosh(-x) = cosh(x) */
    y &= ( IEEESPMantisse_Mask + IEEESPExponent_Mask );
    
    if ( IEEESP_Pinfty == y)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return y;
    }
    
    Res = IEEESPExp(y);
    
    if ( IEEESP_Pinfty == Res )
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0x7f000000; /* Res; */
    }
    
    if ( y < 0x41100000  ) Res = IEEESPAdd(Res, IEEESPDiv(one, Res));
    
    /* Res = Res / 2 */
    Res -= 0x00800000;
    
    if ( 0 == Res || Res < 0 )
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
