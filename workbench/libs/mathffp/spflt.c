/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, SPFlt,

/*  SYNOPSIS */
        AROS_LHA(LONG, inum, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 6, Mathffp)

/*  FUNCTION

    INPUTS

    RESULT
	FFP number

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : ffp is not exactly the integer

    BUGS

    INTERNALS
        Return zero for x == 0.
        If x < 0 set the sign-bit and calculate the absolute value
        of x.
        Find out which bit is the highest-set bit. If the number
        of that bit <code>> 24</code> then the result has the highest bit
        of the mantisse set to one and the exponent equals the
        number of the bit + 2. This is due to the fact that we only
        have 24 bits for the mantisse.
        Otherwise rotate the given integer by
        (32 - (number of highes set bit + 1)) bits to the left and
        calculate the result from that.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
  
    BYTE Exponent = 0;
    LONG TestMask = 0xFFFFFFFF;
    LONG Res = 0;
    
    D(kprintf("SPFlt(%d) = ",inum));
    
    if (inum == 0)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        D(kprintf("0\n"));
        return 0;
    }
    
    if (inum < 0)
    {
        Res = FFPSign_Mask;
        inum = -inum;
    }
    /* find out which is the number of the highes set bit */
    while (TestMask & inum)
    {
        Exponent ++;
        TestMask <<= 1;
    }
    
    /* Exponent = number of highest set bit + 1 */
    
    if (Exponent > 0) /* > 32 bit LONG shift = undefined */
	inum <<= (32 - Exponent);
    else
    	inum = 0;
    if ((char) inum < 0) inum +=0x100;
    inum &= FFPMantisse_Mask;
    
    /* adapt the exponent to the ffp format */
    Exponent += 0x40;
    Res |= inum | Exponent;
    if ((char) Res < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    if (Exponent > (25 + 0x40))
    {
        Res |= 0x80000000;
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    D(kprintf("%x\n",Res));
    
    return Res;
    
    AROS_LIBFUNC_EXIT
}
