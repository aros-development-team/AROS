/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate the hyperbolic sine of the IEEE single precision number

    RESULT
      IEEE single precision floating point number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big for IEEE single precsion format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      <code>
      sinh(x) = (1/2)*( e^x- e^(-x) )

      sinh( |x| >=  9 ) = (1/2) * (e^x);
      </code>
    HISTORY
*/

AROS_LH1(float, IEEESPSinh,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 10, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    LONG y2 = y & (IEEESPMantisse_Mask + IEEESPExponent_Mask);
    LONG tmp;
    
    if ( IEEESP_Pinfty == y2)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return y;
    }
    
    /* sinh(-x) = -sinh(x) */
    Res = IEEESPExp(y2);
    
    if ( IEEESP_Pinfty == Res )
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }
    
    if ( y2 < 0x41100000 )
    {
        /* 
            the following lines is neccessary or otherwise changes in
            the defines/mathieeesing*.h-files would have to be made!
        */
        tmp = IEEESPDiv(one, Res);
        Res = IEEESPAdd(Res, tmp | IEEESPSign_Mask );
    }
    /* Res = Res / 2 */
    Res -= 0x00800000;
    
    /* at this point Res has to be positive to be valid */
    if ( Res <= 0)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (y & IEEESPSign_Mask);
    }
    
    if ( y < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (Res | IEEESPSign_Mask);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
