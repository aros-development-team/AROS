/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*
    FUNCTION
        Calculate the sum of two ffp numbers

    RESULT
        sum of fnum1 and fnum2.

        Flags:
	<description>
	<li><item>zero</item>result is zero</li>
        <li><item>negative</item>result is negative</li>
        <li><item>overflow</item>result is too large or too small for ffp format</li>
	</description>

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
        <p>Adapt the exponent of the ffp-number with the smaller
        exponent to the ffp-number with the larger exponent.
        Therefore rotate the mantisse of the ffp-number with the
        smaller exponents by n bits, where n is the absolute value
        of the difference of the exponents.</p>

        <p>The exponent of the target ffp-number is set to the larger
        exponent plus 1.</p>

        <p>Additionally rotate both numbers by one bit to the right so
        you can catch a result &gt; 1 in the MSB.</p>

        <p>If the signs of the two numbers are equal then simply add
        the two mantisses. The result of the mantisses will be
        [0.5 .. 2[. Check the MSB. If zero, then the result is &lt; 1
        and therefore subtract 1 from the exponent. Normalize the
        mantisse of the result by rotating it one bit to the left.
        Check the mantisse for 0.</p>

        <p>If the signs of the two numbers are different then subtract
        the ffp-number with the neagtive sign from the other one.
        The result of the mantisse will be [-1..1[. If the MSB of
        the result is set, then the result is below zero and therefore
        you have to calculate the absolute value of the mantisse.
        Check the mantisse for zero. Normalize the mantisse by
        rotating it to the left and decreasing the exponent for every
        rotation.</p>

        <p>Test the exponent of the result for an overflow.
        That`s it!</p>

    HISTORY
*/

AROS_LH2(float, SPAdd,
    AROS_LHA(float, fnum1, D1),
    AROS_LHA(float, fnum2, D0),
    struct LibHeader *, MathBase, 11, Mathffp
)
{
    AROS_LIBFUNC_INIT
    
    LONG Res;
    ULONG Mant1, Mant2;
    char Shift;
    char Exponent;
    
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    
    Mant1 = fnum1 & FFPMantisse_Mask;
    Mant2 = fnum2 & FFPMantisse_Mask;
    Shift = ((char)fnum1 & FFPExponent_Mask) -
          ((char)fnum2 & FFPExponent_Mask);
    
    if (Shift > 0)
    {
        if (Shift >= 31)
        {
            Mant2 = 0;
        }
        else
        {
            Mant2 >>= (Shift + 1);
        }
        Mant1 >>= 1;
        Exponent = (fnum1 & FFPExponent_Mask) + 1;
    }
    else
    {
        if (Shift <= -31)
        {
            Mant1 = 0;
        }
        else
        {
            Mant1 >>= (-Shift + 1);
        }
        Mant2 >>= 1;
        Exponent = (fnum2 & FFPExponent_Mask) + 1;
    }
    
    /* sign(fnum1) == sign(fnum2)
    ** simple addition
    ** 0.5 <= res < 2
    */
    if ( ((BYTE) fnum1 & FFPSign_Mask) - ((BYTE) fnum2 & FFPSign_Mask) == 0)
    {
        Res = fnum1 & FFPSign_Mask;
        Mant1 += Mant2;
        if ((LONG) Mant1 > 0)
        {
            Exponent --;
            Mant1 +=Mant1;
        }
    
        if (0 == Mant1)
        {
            SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return 0;
        }
    }
    /* second case: sign(fnum1) != sign(fnum2)
    ** -1 <= res < 1
    */
    else
    {
        if ((char) fnum1 < 0)
        {
            Mant1 = Mant2 - Mant1;
        }
        else /* fnum2 < 0 */
        {
            Mant1 = Mant1 - Mant2;
        }
        /* if the result is below zero */
        if ((LONG) Mant1 < 0)
        {
            Res = FFPSign_Mask;
            Mant1 =-Mant1;
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        }
        else
        {
            Res = 0;
        }
        /* test the result for zero, has to be done before normalizing
        ** the mantisse
        */
        if (0 == Mant1)
        {
            SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
            return 0;
        }
        /* normalize the mantisse */
        while ((LONG) Mant1 > 0)
        {
            Mant1 += Mant1;  /* one bit to the left. */
            Exponent--;
        }    
    } /* else */
    
    if ((char) Exponent < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit);
        /* do NOT change Negative_Bit! */
        return (Res | (FFPMantisse_Mask | FFPExponent_Mask));
    }
    
    Res |= (Mant1 & FFPMantisse_Mask) | Exponent;
    
    kprintf("SPAdd(%x,%x)=%x\n",fnum1,fnum2,Res);
    
    return Res;

    AROS_LIBFUNC_EXIT
}
