/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate y raised to the x power (y^x)

    RESULT
      IEEE single precision floating point number


      flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH2(float, IEEESPPow,
    AROS_LHA(float, x, D1),
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 15, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    /* 
        a ^ b  = e^(b * ln a )
        y ^ x  = e^(x * ln y )
    */
    LONG Res;
    
    /* y^x is illegal if y<0 and x is not an integer-value */
    if (y < 0 && x != IEEESPCeil(x) ) return 0;
    
    Res = IEEESPLog( y & (IEEESPMantisse_Mask + IEEESPExponent_Mask) );
    Res = IEEESPMul(Res, x);
    Res = IEEESPExp(Res);
    
    /* 
        if y < 0 and x was and even integer, the result is positive, otherwise
        it is negative.
    */
    if (y < 0 && TRUE == intern_IEEESPisodd(x) ) Res |= IEEESPSign_Mask;
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
    
    if ( Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    if ( IEEESP_Pinfty == (Res & (IEEESPMantisse_Mask + IEEESPExponent_Mask)) )
    {
        /* don`t touch the Negative_Bit now!*/
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
