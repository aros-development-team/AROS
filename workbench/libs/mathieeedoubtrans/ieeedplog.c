/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPLog,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 14, MathIeeeDoubTrans)

/*  FUNCTION
        Calculate logarithm (base e) of the given IEEE double precision number

    INPUTS

    RESULT
        IEEE double precision number

        flags:
        zero     : result is zero
        negative : result is negative
        overflow : argument was negative

    BUGS

    INTERNALS
        ALGORITHM:

        If the Argument is negative set overflow-flag and return NAN.
        If the Argument is 0 return 0xFFF0000000000000.
        If the Argument is pos. Infinity return pos. Infinity.

        All other cases:

        (ld is the logarithm with base 2)
        (ln is the logarithm with base e)
        y = M * 2^E

        ln y = ln ( M * 2^E ) =

           = ln M + ln 2^E =

           = ln M + E * ln (2) =

             ld M        ld 2
           = ----- + E * ----- =      [ld 2 = 1]
             ld e        ld e

             ld M + E
           = --------
             ld e

        ld e can be precalculated, of course.
        For calculating ld M see file intern_ieeespld.c

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    QUAD Res, tmp, Exponent64, ld_M;
    LONG Exponent;
    /* check for negative sign */
    if ( is_lessSC(y, 0x0, 0x0) /* y<0 */)
    {
        SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        Set_Value64C(Res, IEEEDPNAN_Hi, IEEEDPNAN_Lo);
        return Res;
    }
    
    if ( is_eqC(y, 0x0, 0x0) )
    {
        Set_Value64C
        (
            Res, 
            (IEEEDPSign_Mask_Hi + IEEEDPExponent_Mask_Hi),
            (IEEEDPSign_Mask_Lo + IEEEDPExponent_Mask_Lo)
        );
        return Res;
    }
    /* check for argument == 0 or argument == +infinity */
    if
    ( 
           is_eqC(y, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo) 
        || is_eqC(y, IEEEDPExponent_Mask_Hi, IEEEDPExponent_Mask_Lo)
    )
    {
        return y;
    }
    
    /* convert the Exponent of the argument (y) to the ieeedp-format */
    Exponent = ((Get_High32of64(y) & IEEEDPExponent_Mask_Hi) >> 20) - 0x3fe;
    Exponent64 = IEEEDPFlt(Exponent);
    
    Set_Value64(tmp, y);
    AND64QC(tmp, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    OR64QC(tmp, 0x3fe00000, 0x0);
    ld_M = intern_IEEEDPLd
    ( 
        (struct MathIeeeDoubTransBase *) MathIeeeDoubTransBase,
        tmp
    );
    /*      
                      ld M + E
        ln(fnum1) =  --------
                        ld e
    */
    Set_Value64C(tmp, 0x3fe62e42, 0xfefa39ef ); /* ln 2*/
    return IEEEDPMul( IEEEDPAdd(ld_M, Exponent64), tmp);

    AROS_LIBFUNC_EXIT
}
