/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPSqrt,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 16, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate square root of IEEE double precision floating point number

    INPUTS

    RESULT
        Motorola fast floating point number

        flags:
        zero     : result is zero
        negative : 0
        overflow : square root could not be calculated

    BUGS

    INTERNALS
        ALGORITHM:
        First check for a zero and a negative argument and take
        appropriate action.

        fnum = M * 2^E

        Exponent is an odd number:

        fnum = ( M*2 ) * 2^ (E-1)
        Now E' = E-1 is an even number and
        -> sqrt(fnum) = sqrt(M)   * sqrt(2)   * sqrt (2^E')
                      = sqrt(M)   * sqrt(2)   * 2^(E'/2)
        (with sqrt(M*2)>1)
                      = sqrt(M)   * sqrt(2)   * 2^(E'/2)
                      = sqrt(M)   * 1/sqrt(2) * 2^(1+(E'/2))
                      = sqrt(M/2)             * 2^(1+(E'/2))

        Exponent is an even number:

        -> sqrt(fnum) = sqrt(M) * sqrt (2^E) =
                      = sqrt(M) * 2^(E/2)

        Now calculate the square root of the mantisse.
        The following algorithm calculates the square of a number + delta
        and compares it to the mantisse. If the square of that	number +
        delta is less than the mantisse then keep that number + delta.
        Otherwise calculate a lower offset and try again.
        Start out with number = 0;</p>

        Exponent = -1;
        Root = 0;
        repeat
        {
            if ( (Root + 2^Exponent)^2 < Mantisse)
            Root += 2^Exponent
            Exponent --;
        }

        until you`re happy with the accuracy

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD Res, ResSquared, Delta, X, TargetMantisse, y2, tmp;
    int z;
    ULONG Exponent;
    
    if (is_eqC(y, 0, 0)) /* 0 == y */
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Set_Value64C(Res, 0, 0);
        return Res;
    }
    /* is fnum negative? */
    if (is_lessSC(y, 0, 0)) /* y < 0 */
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Set_Value64C(Res, IEEEDPNAN_Hi, IEEEDPNAN_Lo);
        return Res;
    }
    
    /* we can calculate the square-root now! */
    
    Set_Value64(y2, y);
    AND64QC(y2, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    OR64QC(y2, 0x00100000, 0x00000000 );
    SHL64(TargetMantisse, y2, 11);
    
    Exponent = (Get_High32of64(y) >> 1) & IEEEDPExponent_Mask_Hi;
    
    /* do we have an odd exponent? */
    if (Get_High32of64(y) & 0x00100000)
    {
        /* TargetMantisse = TargetMantisse / 2; */
        SHRU64(TargetMantisse, TargetMantisse, 1); /* TargetMantisse >>= 1; */
        Exponent += 0x20000000;
    }
    else
    {
        Exponent += 0x1ff00000;
    }
    
    Set_Value64C(Res,        0x0, 0x0);
    Set_Value64C(ResSquared, 0x0, 0x0);
    z = 0;
    
    /* 
        this calculates the sqrt of the mantisse. It`s short, isn`t it?
        Delta starts out with 0.5, then 0.25, 0.125 etc.
    */
    while ( is_neq(ResSquared, TargetMantisse) && z < 53)
    {
        QUAD Restmp, Deltatmp;
        Set_Value64C(Delta, 0x80000000, 0x00000000);
        SHRU64(Delta, Delta, z); /* Delta >> = z */
        
        /*
            X = (Res+Delta)^2 = Res^2 + 2*Res*Delta + Delta^2
        */
        SHRU64(Restmp, Res, z);     /* Restmp   = Res   >> z */
        z++;
        SHRU64(Deltatmp, Delta, z); /* Deltatmp = Delta >> (z+1) */
        
        /* X = ResSquared + (Res >> z)  + (Delta >> ++z); */
        Set_Value64(X, ResSquared);
        ADD64Q(X, Restmp);
        ADD64Q(X, Deltatmp);
        
        if (is_leq(X, TargetMantisse)) /* X <= TargetMantisse */
        {
            //printf("setting a bit!\n");
            //OUTPUT(X);OUTPUT(TargetMantisse);
            ADD64(Res, Res, Delta );     /* Res += Delta */
            Set_Value64(ResSquared, X);   /* ResSquared = X; */
        }
    }
    
    SHRU64(Res, Res, 11);
    AND64QC(Res, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo );
    SHL32(tmp, Exponent, 32);
    OR64Q(Res, tmp);
    
    return Res;

    AROS_LIBFUNC_EXIT
}
