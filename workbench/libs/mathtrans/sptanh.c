/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate hyperbolic tangens of the ffp number

    RESULT
      Motorola fast floating point number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow : (not possible)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
     <code>

                ( e^x - e^(-x) )
     tanh(x) =  ----------------
                ( e^x + e^(-x) )

     tanh( |x| > 9 ) = 1
     </code>

    HISTORY
*/

AROS_LH1(float, SPTanh,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 12, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    ULONG Res;
    LONG tmp;
    
    tmp = (fnum1 & FFPExponent_Mask) - 0x41;
    
    if ( tmp >= 3  &&  (fnum1 & FFPMantisse_Mask) >= 0x90000000 )
    {
        /* 
            tanh( x > 9 ) =  1
            tanh( x <-9 ) = -1
        */    
        return (one | ( fnum1 & FFPSign_Mask ));
    }
    
    /* tanh(-x) = -tanh(x) */
    Res = SPExp(fnum1 & (FFPMantisse_Mask + FFPExponent_Mask ));
    Res = SPDiv
    ( 
        SPAdd(Res, SPDiv(Res, one)),
        SPAdd(Res, (ULONG)SPDiv(Res, one) | FFPSign_Mask )
    );
    
    /* Result is zero */
    if (0 == Res )
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    /* Argument is negative -> result is negative */
    if ( (char) fnum1 < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (Res | FFPSign_Mask );
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
