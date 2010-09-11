/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*
    FUNCTION
        Convert ieeesp-number to integer

    RESULT
        absolute value of y

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : ieeesp out of integer-range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY
*/

AROS_LH1(LONG, IEEESPFix,
    AROS_LHA(float, y, D0),
    struct LibHeader *, MathIeeeSingBasBase, 5, Mathieeesingbas
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    LONG Shift;
    
    if ((y & IEEESPExponent_Mask) > 0x60000000 )
    {
        if(y < 0) /* don`t hurt the SR! */
        {
            SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0x80000000;
        }
        else
        {
            SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0x7fffffff;
        }
    }
    
    if (0 == y || 0x80000000 == y) /* y=+-0; */
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    } 
    
    Shift = (y & IEEESPExponent_Mask) >> 23;
    Shift -=0x7e;
    
    if ((char) Shift >= 25)
    {
        Res = ((y & IEEESPMantisse_Mask) | 0x00800000)  << (Shift-24);
    }
    else
    {
        Res = ((y & IEEESPMantisse_Mask) | 0x00800000)  >> (24 - Shift);
    }
    
    /* Test for a negative sign  */
    if (y < 0)
    {
        Res = -Res;
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return Res;
    
    AROS_LIBFUNC_EXIT
}
