/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculates the angle of a given number representing the tangent
      of that angle. The angle will be in radians.


    RESULT
      IEEE single precision floating point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH1(float, IEEESPAtan,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 5, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG yabs = y & (IEEESPMantisse_Mask | IEEESPExponent_Mask);
    LONG ysquared, ycubed;
    
    /* check for +- infinity -> output: +-pi/2 */
    if (IEEESP_Pinfty == yabs) return (pio2 | (y & IEEESPSign_Mask));
    
    /* atan(0) = 0 */
    if (0 == yabs)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    /* atan(x >= 128) = pi/2 - 1/x */
    if ( (yabs & IEEESPExponent_Mask) >= 0x43000000)
    {
        if (yabs == y) /* arg has ppositive sign */
        {
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            return IEEESPSub(pio2,IEEESPDiv(one,yabs));
        }
        else
        {
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return (IEEESPSub(pio2,IEEESPDiv(one,yabs))) | IEEESPSign_Mask;
        }
    }
    
    /* atan(x >= 64) = pi/2 - 1/x +1/(3*x^3) */
    
    ysquared = IEEESPMul(yabs, yabs);
    
    if( (yabs & IEEESPExponent_Mask) >= 0x42000000)
    {
        ycubed   = IEEESPMul(yabs, ysquared);
        
        /* pi/2 - 1/x + 1/(3*x^3) = pi/2 + (1-3*x^2)/(3*x^3)*/
        if (yabs == y) /* arg has positive sign */
        {
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            return IEEESPAdd
            (
                pio2, IEEESPDiv
                (
                    IEEESPAdd
                    (
                        IEEESPMul(three, ysquared) | IEEESPSign_Mask, one 
                    ),
                    IEEESPMul(three, ycubed)
                )
            );
        }
        else
        {
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return IEEESPAdd
            (
                pio2, IEEESPDiv
                (
                    IEEESPAdd
                    (
                        IEEESPMul(three, ysquared) | IEEESPSign_Mask, one 
                    ),
                    IEEESPMul(three, ycubed)
                )
            ) | IEEESPSign_Mask;
        }
    }
    
    /* atan(x <= 64) */
    return IEEESPAsin(IEEESPDiv(y,IEEESPSqrt(IEEESPAdd(one,ysquared))));
    
    AROS_LIBFUNC_EXIT
} /* IEEESPAtan */
