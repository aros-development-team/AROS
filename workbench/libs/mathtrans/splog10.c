/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate logarithm (base 10) of the given ffp number

    RESULT
      ffp number

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
      If the Argument is 0 return 0xde5bd8fe.

      All other cases:

      (ld is the logarithm with base 2)
      (log is the logarithm with base 10)
      <code>
      fnum1 = M * 2^E

      log fnum1 = log ( M * 2^E ) =

                = log M + log 2^E =

                = log M + E * log (2) =

                  ld M        ld 2
                = ----- + E * ----- =      [ld 2 = 1]
                  ld 10       ld 10

                  ld M + E
                = --------
                  ld 10
      </code>

      ld 10 can be precalculated, of course.
      For calculating ld M see file intern_spld.c

    HISTORY
*/

AROS_LH1(float, SPLog10,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 21, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    LONG ld_M, Exponent, Mask = 0x40, i, Sign;
    
    /* check for negative sign */
    if ((char) fnum1 < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if (0 == fnum1) return 0xde5bd8fe; /* result of the orig. library */
    
    /* convert the Exponent of the argument (fnum1) to the ffp-format */
    Exponent = (fnum1 & FFPExponent_Mask) - 0x40;
    if (Exponent < 0 )
    {
        Exponent = -Exponent;
        Sign = FFPSign_Mask;
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
        
        Exponent <<= (25 + i);
        Exponent |= (0x47 - i + Sign);
    }
    
    ld_M = intern_SPLd((fnum1 & FFPMantisse_Mask) | 0x40);
    
    /*               
                      ld M + E
        log(fnum1) =  --------
                        ld e
    */
    
    return SPMul( SPAdd(ld_M, Exponent), InvLd10);

    AROS_LIBFUNC_EXIT
}
