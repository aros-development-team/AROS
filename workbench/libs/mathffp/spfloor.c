/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, SPFloor,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 15, Mathffp)

/*  FUNCTION
        Calculate the largest integer ffp-number
        less than or equal to fnum

    INPUTS

    RESULT
	FFP number

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0 (???)

    EXAMPLE
       floor(10.5) = 10
       floor(0.5)  = 0
       floor(-0.5) = -1
       floor(-10.5)= -11

    BUGS

    INTERNALS
        ALGORITHM:
        The integer part of a ffp number are the left "exponent"-bits
        of the mantisse!
        Therefore:
        Test the exponent for <= 0. This has to be done separately!
        If the sign is negative then return -1 otherwise return 0.

        Generate a mask of exponent(y) (where y is the given ffp-number)
        bits starting with bit 31.
        If y < 0 then test whether it is already an integer. If not
        then y = y - 1 and generate that mask again. Use the
        mask on the mantisse.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG Mask = 0x80000000;
    
    if (((char)y & FFPExponent_Mask)  <= 0x40)
    {
        if ((char)y < 0)
        {
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0x800000C1; /* -1 */
        }
        else
        {
            SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0;
        }
    }
    
    /* |fnum| >= 1 */
    Mask >>= ( ((char) y & FFPExponent_Mask) - 0x41);
    Mask |= FFPSign_Mask | FFPExponent_Mask;
    
    /* fnum is negative */
    if ((char) y < 0)
    {
        /* is there anything behind the dot? */
        if (0 != (y & (~Mask)) )
        {
            Mask = 0x80000000;
            y    = SPAdd(y, 0x800000c1); /* y = y -1; */
            Mask >>= ((char) y & FFPExponent_Mask) - 0x41;
            Mask |= FFPSign_Mask | FFPExponent_Mask;
        }
    }
    
    if((char) y < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return y & Mask;
    
    AROS_LIBFUNC_EXIT
}
