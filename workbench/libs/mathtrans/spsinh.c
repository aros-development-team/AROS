/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate the hyperbolic sine of the ffp number

    RESULT
      Motorola fast floating point number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big for ffp format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      sinh(x) = (1/2)*( e^x- e^(-x) )

      sinh( |x| >= 44 ) = infinity;
      sinh( |x| >=  9 ) = (1/2) * (e^x);

    HISTORY
*/

AROS_LH1(float, SPSinh,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 10, MathTrans
)
{
    AROS_LIBFUNC_INIT

    ULONG Res;
    LONG tmp;
    
    /* sinh(-x) = -sinh(x) */
    Res = SPExp(fnum1 & (FFPMantisse_Mask + FFPExponent_Mask) );
    
    if ( FFP_Pinfty == Res )
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }
    
    tmp = (fnum1 & FFPExponent_Mask) - 0x41;
    
    if ( tmp <= 2  || (tmp == 3 && (fnum1 & FFPMantisse_Mask) < 0x90000000) )
    {
        Res = SPAdd(Res, ((ULONG)SPDiv(Res, one) | FFPSign_Mask ));
    }
    
    /* Res = Res / 2 */
    
    /* should be (char(Res))-- , but gcc on Linux screws up the result! */
    
    tmp = Res & 0xFFFFFF00;
    Res -= sizeof(char);
    Res = Res | tmp;
    
    if ( 0 == Res || (char)Res < 0 )
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    /* if the argument was negative, the result is also negative */
    if ((char)fnum1 < 0 )
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (Res | FFPSign_Mask);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
