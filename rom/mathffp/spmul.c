/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*
    FUNCTION
        Multiply two ffp numbers
        fnum = fnum1 * fnum2;

    RESULT
	FFP number

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH2(float, SPMul,
    AROS_LHA(float, fnum1, D1),
    AROS_LHA(float, fnum2, D0),
    struct LibHeader *, MathBase, 13, Mathffp
)
{
    AROS_LIBFUNC_INIT
    
    char Exponent = ((char) fnum1 & FFPExponent_Mask) 
                  + ((char) fnum2 & FFPExponent_Mask) - 0x41;
    ULONG Mant1H = ( (ULONG) (fnum1 & FFPMantisse_Mask)) >> 20;
    ULONG Mant2H = ( (ULONG) (fnum2 & FFPMantisse_Mask)) >> 20;
    ULONG Mant1L = (((ULONG) (fnum1 & FFPMantisse_Mask)) >> 8) & 0x00000fff;
    ULONG Mant2L = (((ULONG) (fnum2 & FFPMantisse_Mask)) >> 8) & 0x00000fff;
    LONG Res;
    
    Res  =  (Mant1H * Mant2H) <<  8;
    Res += ((Mant1H * Mant2L) >>  4);
    Res += ((Mant1L * Mant2H) >>  4);
    Res += ((Mant1L * Mant2L) >> 16);
    
    /* Bit 32 is not set */
    if ((LONG)Res > 0)
    {
        Res <<= 1; /* rotate the mantisse by one bit to the left */
    }
    else
    {
        Exponent ++;
    }
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    Res |= ((fnum1 & FFPSign_Mask) ^ (fnum2 & FFPSign_Mask) );
    
    /* overflow? */
    if ((char) Exponent < 0 || (char) Exponent == 0x7f)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        kprintf("%x * %x = %x\n",fnum1,fnum2,Res);
        return (Res | (FFPMantisse_Mask + FFPExponent_Mask));
    }
    
    Res |= Exponent;
    
    if ((char) Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    kprintf("%x * %x = %x\n",fnum1,fnum2,Res);
    
    return Res;
  
    AROS_LIBFUNC_EXIT
}
