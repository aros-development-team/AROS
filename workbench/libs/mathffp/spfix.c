/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(LONG, SPFix,

/*  SYNOPSIS */
        AROS_LHA(float, fnum, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 5, Mathffp)

/*  FUNCTION
        Convert ffp-number to integer

    INPUTS

    RESULT
        absolute value of fnum1

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : ffp out of integer-range

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    BYTE Shift;
    
    if ((fnum & FFPExponent_Mask) > 0x60 )
    {
        if(fnum < 0) /* don`t hurt the SR! */
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
    
    
    Shift = (fnum & FFPExponent_Mask) - 0x40;
    if (Shift > 0) /* > 32 bit LONG shift = undefined */
	Res = ((ULONG)(fnum & FFPMantisse_Mask)) >> (32 - Shift);
    else
    	Res = 0;
    
    if (0 == Res)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if (0x80000000 == Res) return 0x7fffffff;
    
    
    /* Test for a negative sign */
    if ((char)fnum < 0)
    {
        Res = -Res;
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }

    D(kprintf("SPFix(%x) = %d\n", fnum, Res));
    
    return Res;
    
    AROS_LIBFUNC_EXIT
}
