/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate hyperbolic tangens of the IEEE single precision number

    RESULT
      IEEE single precision floating point number

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

     tanh( |x| >= 9 ) = 1
      </code>

    HISTORY
*/

AROS_LH1(float, IEEESPTanh,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 12, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    LONG y2 = y & (IEEESPMantisse_Mask + IEEESPExponent_Mask );
    LONG tmp;
    
    if ( y2 >= 0x41100000 )
    /* 
        tanh( x > 9 ) =  1
        tanh( x <-9 ) = -1
    */
    return (one | ( y & IEEESPSign_Mask ));
    
    /* tanh(-x) = -tanh(x) */
    Res = IEEESPExp(y2);
    tmp = IEEESPDiv(one, Res);
    Res = IEEESPDiv
    ( 
        IEEESPAdd(Res, (tmp | IEEESPSign_Mask) ),
        IEEESPAdd(Res, tmp)
    );
    
    /* Result is zero */
    if (0 == Res )
    {
        if (y < 0)
        {
            SetSR(Zero_Bit | Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        }
        else
        {
            SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        }
        
        return (y & IEEESPSign_Mask);
    }
    
    /* Argument is negative -> result is negative */
    if ( y < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (Res | IEEESPSign_Mask );
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
