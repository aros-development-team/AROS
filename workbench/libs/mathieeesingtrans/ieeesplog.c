/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*
    FUNCTION
      Calculate logarithm (base e) of the given IEEE single precision number

    RESULT
      IEEE single precision number

      flags:
        zero     : result is zero
        negative : result is negative
        overflow : argument was negative

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
      ALGORITHM:

      If the Argument is negative set overflow-flag and return 0.
      If the Argument is 0 return 0xffffffff.

      All other cases:

      (ld is the logarithm with base 2)
      (ln is the logarithm with base e)
      y = M * 2^E

      <code>
      ln y = ln ( M * 2^E ) =

           = ln M + ln 2^E =

           = ln M + E * ln (2) =

             ld M        ld 2
           = ----- + E * ----- =      [ld 2 = 1]
             ld e        ld e

             ld M + E
           = --------
             ld e
      </code>

      ld e can be precalculated, of course.
      For calculating ld M see file intern_ieeespld.c

    HISTORY
*/

AROS_LH1(float, IEEESPLog,
    AROS_LHA(float, y, D0),
    struct Library *, MathIeeeSingTransBase, 14, MathIeeeSingTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG ld_M, Exponent, Mask = 0x40, i, Sign;
    
    /* check for negative sign */
    if ( y < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    /* check for argument == 0 or argument == +infinity */
    if (0 == y || IEEESP_Pinfty == y) return y;
    
    /* convert the Exponent of the argument (y) to the ieeesp-format */
    Exponent = ((y & IEEESPExponent_Mask) >> 23) - 0x7e ;
    
    if (Exponent < 0 )
    {
        Exponent =-Exponent;
        Sign = IEEESPSign_Mask;
    }
    else
    {
        Sign = 0;
    }
    
    /* find the number of the highest set bit in the exponent */
    if (Exponent != 0)
    {
        i = 0;
        while ( (Mask & Exponent) == 0)
        {
            i ++;
            Mask >>= 1;
        }
        
        Exponent <<= (17 + i);
        Exponent &= IEEESPMantisse_Mask;
        Exponent |= ((0x85 - i ) << 23);
        Exponent |= Sign;
    }
    
    ld_M = intern_IEEESPLd((y & IEEESPMantisse_Mask) | 0x3f000000);
    
    /*               
                      ld M + E
        log(fnum1) =  --------
                       ld e
    */
    
    return IEEESPMul( IEEESPAdd(ld_M, Exponent), InvLde);

    AROS_LIBFUNC_EXIT
}
