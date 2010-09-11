/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*
    FUNCTION
        Divide two ffp numbers
        fnum = fnum2 / fnum1;

    RESULT
        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS
        The parameters are swapped !

    SEE ALSO


    INTERNALS
      ALGORITHM:
        Check if fnum2 == 0: result = 0;
        Check if fnum1 == 0: result = overflow;
        The further algorithm comes down to a pen &amp; paper division

    HISTORY
*/

AROS_LH2(float, SPDiv,
    AROS_LHA(float, fnum1, D1),
    AROS_LHA(float, fnum2, D0),
    struct LibHeader *, MathBase, 14, Mathffp
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res = 0;
    char Exponent = ((char) fnum2 & FFPExponent_Mask) -
                  ((char) fnum1 & FFPExponent_Mask) + 0x41;
    
    LONG Mant2 = ((ULONG)fnum2 & FFPMantisse_Mask);
    LONG Mant1 = ((ULONG)fnum1 & FFPMantisse_Mask);
    ULONG Bit_Mask = 0x80000000;

    /* check if the dividend is zero */
    if (0 == fnum2)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    /* check for division by zero */
    if (0 == fnum1)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
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
        Exponent --;
    }

    if ((char) Res < 0)
    {
        Res += 0x00000100;
    }
    
    Res &= FFPMantisse_Mask;
    Res |= (Exponent & 0x7f);
    Res |= (fnum1 & FFPSign_Mask) ^ (fnum2 & FFPSign_Mask);

    if ((char) Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    }
    
    if ((char) Exponent < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return(Res | (FFPMantisse_Mask | FFPExponent_Mask));
    }

    kprintf("%x / %x =%x\n",fnum2,fnum1,Res);
  
    return Res;

    AROS_LIBFUNC_EXIT
}
