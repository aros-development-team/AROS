/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathtrans_intern.h"

/*
    FUNCTION
      Calculate square root of ffp number

    RESULT
      Motorola fast floating point number

      flags:
         zero     : result is zero
         negative : 0
         overflow : square root could not be calculated

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

      ALGORITHM:
        First check for a zero and a negative argument and take
        appropriate action.
        fnum1 = M * 2^E

        If exponent is an odd number:
     <code>
          fnum = ( M*2 ) * 2^ (E-1)
          Now E' = E-1 is an even number and
               -> sqrt(fnum) = sqrt(M)   * sqrt(2)   * sqrt (2^E')
                             = sqrt(M)   * sqrt(2)   * 2^(E'/2)
           (with sqrt(M*2)>1)
                             = sqrt(M)   * sqrt(2)   * 2^(E'/2)
                             = sqrt(M)   * 1/sqrt(2) * 2^(1+(E'/2))
                             = sqrt(M/2)             * 2^(1+(E'/2))
     </code>


       If Exponent is an even number:
     <code>
           -> sqrt(fnum) = sqrt(M) * sqrt (2^E) =
                         = sqrt(M) * 2^(E/2)
     </code>

        Now calculate the square root of the mantisse.
        The following algorithm calculates the square of a number + delta
        and compares it to the mantisse. If the square of that  number +
        delta is less than the mantisse then keep that number + delta.
        Otherwise calculate a lower delta and try again.
        Start out with number = 0;


     <code>
        Exponent = -1;
        Root = 0;
        repeat
        {
          if ( ( Root + 2^Exponent ) ^2 < Mantisse)
            Root += 2^Exponent
          Exponent --;
        }
        until you`re happy with the accuracy
     </code>


    HISTORY
*/

AROS_LH1(float, SPSqrt,
    AROS_LHA(float, fnum1, D0),
    struct Library *, MathTransBase, 16, MathTrans
)
{
    AROS_LIBFUNC_INIT
    
    ULONG Res, ResSquared, Delta, X, TargetMantisse;
    int z;
    BYTE Exponent;
    
    if (0 == fnum1)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    /* is fnum negative */
    if ((char)fnum1 < 0)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return fnum1;
    }
    
    /* let us calculate the square-root now! */
    
    TargetMantisse = ((ULONG)fnum1 & FFPMantisse_Mask);
    
    if (fnum1 & 1)
    {
        /* TargetMantisse = TargetMantisse / 2; */
        TargetMantisse >>= 1;
        Exponent = ((BYTE)fnum1 >> 1) + 0x21;
    }
    else
    {
        Exponent = ((BYTE)fnum1 >> 1) + 0x20;
    }
    
    Res = 0;
    ResSquared = 0;
    z = 0;
    
    /* 
        this calculates the sqrt of the mantisse. It`s short, isn`t it?
        Delta starts out with 0.5, then 0.25, 0.125 etc.
    */
    while (ResSquared != TargetMantisse && z < 25)
    {
        Delta = (0x80000000 >> z);
        /* 
            X = (Res+Delta)^2 =
              = Res^2      + 2*Res*Delta + Delta^2
        */
        X = ResSquared + (Res >> z)  + (Delta >> (z+1));
        z++;
        if (X <= TargetMantisse)
        {
            Res += Delta;
            ResSquared = X;
        }
    }
    
    /* an adjustment for precision */
    if ((char) Res < 0) Res += 0x100;
    
    Res &= FFPMantisse_Mask;
    Res |= Exponent;
    
    return Res;

    AROS_LIBFUNC_EXIT
}
