/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, IEEESPFloor,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct LibHeader *, MathIeeeSingBasBase, 15, Mathieeesingbas)

/*  FUNCTION
        Calculate the largest integer ieeesp-number
        less than or equal to fnum

    INPUTS

    RESULT
	IEEE single precision floating point

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0 (???)

    EXAMPLE
       IEEESPFloor(10.5) = 10
       IEEESPFloor(0.5)  = 0
       IEEESPFloor(-0.5) = -1
       IEEESPFloor(-10.5)= -11

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG Mask = 0x80000000;

    if (0x7f880000 == y) return y;

    if ((y & IEEESPExponent_Mask)  < 0x3f800000)
    {
        if (y < 0) /* negative sign? */
        {
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0xbf800000; /* -1 */
        }
        else
        {
            SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0;
        }
    }
    
    /* |fnum| >= 1 */
    Mask >>= ((y & IEEESPExponent_Mask) >> 23) - 0x77;
    
    /* y is negative */
    if (y < 0)
    {
        /* is there anything behind the decimal dot? */
        if (0 != (y & (~Mask)) )
        {
            y    = IEEESPAdd(y, 0xbf800000 ); /* fnum = fnum -1; */
            Mask = 0x80000000;
            Mask >>= ((y & IEEESPExponent_Mask) >> 23) - 0x77;
        }
    }
    
    if(y < 0) SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    
    return y & Mask;

    AROS_LIBFUNC_EXIT
}
