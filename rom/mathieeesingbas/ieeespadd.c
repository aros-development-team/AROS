/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*
    FUNCTION
        Calculate the sum of two IEEE single precision numbers

    RESULT
        sum of y and z

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is too large or too small for IEEESP format

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY
*/

AROS_LH2(float, IEEESPAdd,
    AROS_LHA(float, y, D0),
    AROS_LHA(float, z, D1),
    struct LibHeader *, MathIeeeSingBasBase, 11, Mathieeesingbas
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    LONG Mant1, Mant2;
    LONG Shift;
    LONG Exponent;
    
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    
    Shift = ((y & IEEESPExponent_Mask) -
           (z & IEEESPExponent_Mask)) >> 23;
    
    if (y != 0 && y != IEEESPSign_Mask )
    {
        Mant1 = (y & IEEESPMantisse_Mask) | 0x00800000;
    }
    else
    {
        Mant1 = 0;
    }
    
    if (z != 0 && z != IEEESPSign_Mask )
    {
        Mant2 = (z & IEEESPMantisse_Mask) | 0x00800000;
    }
    else
    {
        Mant2 = 0;
    }

    if (Shift > 0)
    {
        Mant2 >>= Shift;
        Exponent = (y & IEEESPExponent_Mask);
    }
    else
    {
        Mant1 >>= (-Shift);
        Exponent = (z & IEEESPExponent_Mask);
    }

    /* sign(fnum1) == sign(fnum2)
    ** simple addition
    ** 0.25 <= res < 1
    */
    if ( (z & IEEESPSign_Mask) - ( y & IEEESPSign_Mask) == 0)
    {
        Res = y & IEEESPSign_Mask;
        Mant1 += Mant2;
        if ( (Mant1 & 0x01000000) != 0)
        {
            Exponent += 0x00800000;
            Mant1 >>= 1;
        }
        Mant1 &= IEEESPMantisse_Mask;
    }
    /* second case: sign(fnum1) != sign(fnum2)
    **   -1 <= res < 1
    */
    else
    {
        if ( y < 0)
        {
            Mant1 = Mant2 - Mant1;
        }
        else /* fnum2 < 0 */
        {
            Mant1 = Mant1 - Mant2;
        }
        /*if the result is below zero */
        if (  Mant1 < 0)
        {
            Res = IEEESPSign_Mask;
            Mant1 =-Mant1;
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        }
        else
        {
            Res = 0;
        }
        
        if (0 == Mant1)
        {
            SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0;
        }
        else
        {
            /* normalize the mantisse */
            while ( (Mant1 & 0x00800000) == 0)
            {
                Mant1 += Mant1;  /*one bit to the left. */
                Exponent -= 0x00800000;
            }
            Mant1 &= IEEESPMantisse_Mask;
        } /* else */
    
    } /* else */

    if ( Exponent < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit);
        /* do NOT change Negative_Bit! */
        return (Res | (IEEESPMantisse_Mask | IEEESPExponent_Mask));
    }
    
    Res |= Mant1 | Exponent;
    return Res;

    AROS_LIBFUNC_EXIT
}
