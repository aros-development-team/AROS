/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(float, SPPow,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D1),
        AROS_LHA(float, fnum2, D0),

/*  LOCATION */
        struct Library *, MathTransBase, 15, MathTrans)

/*  FUNCTION
        Calculate fnum2 raised to the fnum1 power (fnum2^fnum1)

    INPUTS

    RESULT
        Motorola fast floating point number

        flags:
        zero     : result is zero
        negative : result is negative
        overflow : result is too big

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    /* 
        a     ^ b     = e^(b     * ln a    )
       fnum2 ^ fnum1 = e^(fnum1 * ln fnum2)
    */
    
    ULONG Res;
    Res = SPLog( fnum2 & (FFPMantisse_Mask + FFPExponent_Mask) );
    Res = SPMul(Res, fnum1);
    Res = SPExp(Res);
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if ((char) Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
