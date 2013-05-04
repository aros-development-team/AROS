/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, IEEESPLog10,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct Library *, MathIeeeSingTransBase, 21, MathIeeeSingTrans)

/*  FUNCTION
        Calculate logarithm (base 10) of the given IEEE single precision number

    INPUTS

    RESULT
        IEEE single precision number

        flags:
        zero     : result is zero
        negative : result is negative
        overflow : argument was negative

    BUGS

    INTERNALS
        ALGORITHM:

        If the Argument is negative set overflow-flag and return 0.
        If the Argument is 0 return 0xffffffff.

        All other cases:

        (ld is the logarithm with base 2)
        (log is the logarithm with base 10)
        y = M * 2^E

        log y = log ( M * 2^E ) =

            = log M + log 2^E =

            = log M + E * log (2) =

              ld M        ld 2
            = ----- + E * ----- =      [ld 2 = 1]
              ld 10       ld 10

              ld M + E
            = --------
              ld 10

        ld 10 can be precalculated, of course.
        For calculating ld M see file intern_ieeespld.c

*****************************************************************************/
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
                       ld 10
    */
    
    return IEEESPMul( IEEESPAdd(ld_M, Exponent), InvLd10);
    
    AROS_LIBFUNC_EXIT
}
