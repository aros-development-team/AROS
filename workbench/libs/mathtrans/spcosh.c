/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate the hyperbolic cosine of the ffp number

    RESULT
      Motorola fast floating point number

      flags:
        zero     : result is zero
        negative : 0 (not possible)
        overflow : result too big for ffp-number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
     <code>
      cosh(x) = (1/2)*( e^x + e^(-x) )

      cosh( |x| >= 44 ) = infinity;
      cosh( |x| >= 9  ) = (1/2) * (e^x);
     </code>

    HISTORY
*/

AROS_LH1(float, SPCosh,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 11, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    ULONG Res;
    LONG tmp;
    
    /* cosh(-x) = cosh(x) */
    fnum1 &= ( FFPMantisse_Mask + FFPExponent_Mask );
    
    Res = SPExp(fnum1);
    
    if ( FFP_Pinfty == Res )
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return Res;
    }
    
    tmp = (fnum1 & FFPExponent_Mask) - 0x41;
    
    if ( tmp <= 2 || (tmp == 3 && (fnum1 & FFPMantisse_Mask) < 0x90000000) )
    {
        Res = SPAdd(Res, SPDiv(Res, one));
    }
    
    /* Res = Res / 2 */
    /* should be ((char)Res) --, but gcc on Linux screws up the result  */
    tmp = Res & 0xFFFFFF00;
    Res -= sizeof(char); 
    Res = tmp | Res;
    
    if (0 == Res || (char)Res < 0 )
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
