/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(double, IEEEDPFlt,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 6, MathIeeeDoubBas)

/*  FUNCTION
        Convert an integer into an IEEE double precision floating point
        number.

    INPUTS
        y - a 32-bit integer.

    RESULT
        x - IEEE double precision floating point number.

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IEEEDPFix()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG Exponent = 0;
    LONG TestMask = 0xFFFFFFFF;
    QUAD Res, yQuad, ExponentQuad;
    double * DRes = (double *)&Res;
    
    Set_Value64C(Res,0,0);
    
    if (0 == y)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return *DRes; /* return 0 */
    }
    
    if (y < 0 )
    {
        Set_Value64C(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
        y = -y;
    }
    /* find out which is the number of the highest set bit */
    while (TestMask & y)
    {
        Exponent ++;
        TestMask <<= 1;
    }
    
    SHL32(yQuad , y , (53 - Exponent) );
    
    AND64QC(yQuad,  IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    
    Exponent += 0x3fe;
    
    /* adapt Exponent to IEEEDP-Format */
    SHL32(ExponentQuad, Exponent, 52);
    OR64Q(Res, yQuad);
    OR64Q(Res, ExponentQuad);
    if ( is_lessSC(Res,0,0) ) /* Res < 0 */
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return *DRes;

    AROS_LIBFUNC_EXIT
}
