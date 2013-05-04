/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, SPTieee,

/*  SYNOPSIS */
        AROS_LHA(float, fnum, D0),

/*  LOCATION */
        struct Library *, MathTransBase, 17, MathTrans)

/*  FUNCTION
        Convert FFP number to single precision ieee number

    INPUTS

    RESULT
        IEEE Single Precision Floating Point

        flags:
        zero     : result is zero
        negative : result is negative
        overflow : exponent of the ieee-number was out of range for ffp

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    LONG Exponent;
    if (0 == fnum)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    Exponent = (fnum & FFPExponent_Mask) - 0x40 + 126;
    
    Res = ( Exponent << (30-7) );
    Res |= (((ULONG)fnum & 0x7fffff00) >> 8);
    
    if ((char) fnum < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Res |= IEEESPSign_Mask;
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
