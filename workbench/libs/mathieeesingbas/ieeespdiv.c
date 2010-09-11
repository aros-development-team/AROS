/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*
    FUNCTION
        Divide two IEEE single precision floating point numbers
        x = y / z;

    RESULT

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:<br/>
        Check if fnum2 == 0: result = 0;<br/>
        Check if fnum1 == 0: result = overflow;<br/>
        The further algorithm comes down to a pen &amp; paper division.
    HISTORY
*/

AROS_LH2(float, IEEESPDiv,
    AROS_LHA(float, y, D0),
    AROS_LHA(float, z, D1),
    struct LibHeader *, MathIeeeSingBasBase, 14, Mathieeesingbas
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res = 0;
    LONG Exponent = (y & IEEESPExponent_Mask) 
                  - (z & IEEESPExponent_Mask) + 0x3f800000;
    
    LONG Mant2 = ((y & IEEESPMantisse_Mask) | 0x00800000) << 8;
    LONG Mant1 = ((z & IEEESPMantisse_Mask) | 0x00800000) << 8;
    ULONG Bit_Mask = 0x80000000;

    if (0 == z && 0 == y) return 0x7f880000;

    /* check if the dividend is zero */
    if (0 == z)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (IEEESPExponent_Mask | ((y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask)));
    }

    /* check for division by zero */
    if (0 == y)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return (y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask);
    }

    while (Bit_Mask >= 0x40 && Mant2 != 0)
    {
        if (Mant2 - Mant1 >= 0)
        {
            Mant2 -= Mant1;
            Res |= Bit_Mask;
            
            while (Mant2 > 0)
            {
                Mant2 <<= 1;
                Bit_Mask >>= 1;
            }
            
            while (Mant1 > 0)
            {
                Mant1 <<=1;
                Bit_Mask <<=1;
            }
        } /* if */
        else
        {
            Mant1 = (ULONG) Mant1 >> 1;
            Bit_Mask >>= 1;
        }
    } /* while */

    /* normalize the mantisse */
    while (Res > 0)
    {
        Res += Res;
        Exponent -=0x00800000;
    }

    if ((char) Res < 0) Res += 0x00000100;
    Res >>= 8;

    Res &= IEEESPMantisse_Mask;
    Res |= Exponent;
    Res |= (y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask);

    if (Res < 0) SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    if (Exponent < 0) SetSR(Overflow_Bit, Negative_Bit | Overflow_Bit);

    return Res;

    AROS_LIBFUNC_EXIT
}
