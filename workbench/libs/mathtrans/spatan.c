/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculates the angle of a given number representing the tangent
      of that angle. The angle will be in radians.

    RESULT
      Motorola fast floating point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH1(float, SPAtan,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 5, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG fnumabs = fnum1 & (FFPMantisse_Mask | FFPExponent_Mask);
    LONG fnumsquared, fnumcubed;
    
    /* check for +- infinity -> output: +-pi/2 */
    if ((LONG)FFP_Pinfty == fnumabs ) return (pio2 | (fnum1 & FFPSign_Mask));
    
    /* atan(0) = 0 */
    if (0 == fnumabs)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    /* atan(x>= 128) = pi/2 - 1/x */
    if ((BYTE) fnumabs >= 0x48)
    {
        if (fnumabs == fnum1) /* arg has ppositive sign */
        {
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            return SPSub(SPDiv(fnumabs,one),pio2);
        }
        else
        {
            LONG res;
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            
            /* workaround a bug in egcs 1.0.3: complains about illegal operands to | */
            res = SPSub(SPDiv(fnumabs,one),pio2);
            return res | FFPSign_Mask;
        }
    }
    
    /* atan(x >= 64) = pi/2 - 1/x +1/(3*x^3) */
    
    fnumsquared = SPMul(fnumabs, fnumabs);
    
    if((BYTE) fnumabs >= 0x47)
    {
        fnumcubed   = SPMul(fnumabs, fnumsquared);
        
        /* pi/2 - 1/x + 1/(3*x^3) = pi/2 + (1-3*x^2)/(3*x^3)*/
        if (fnumabs == fnum1) /* arg has positive sign */
        {
            LONG res;
            SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
            
            /* workaround a bug in egcs 1.0.3: complains about illegal operands to | */
            res = SPMul(three, fnumsquared);
            return SPAdd
            (
                pio2, SPDiv
                (
                    SPMul(three, fnumcubed),
                    SPAdd(res | FFPSign_Mask, one)
                )
            );
        }
        else
        {
            LONG res;
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            
            /* workaround a bug in egcs 1.0.3: complains about illegal operands to | */
            res = SPMul(three, fnumsquared);
            res = SPAdd
            (
                pio2, SPDiv
                ( 
                    SPMul(three, fnumcubed), 
                    SPAdd( res | FFPSign_Mask, one )
                )
            );
            return res | FFPSign_Mask;
        }
    }
    
    /* atan(x <= 64) */
    return SPAsin(SPDiv(SPSqrt(SPAdd(one,fnumsquared)),fnum1));

    AROS_LIBFUNC_EXIT
}
