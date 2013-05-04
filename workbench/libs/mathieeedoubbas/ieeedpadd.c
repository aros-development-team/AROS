/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD2(double, IEEEDPAdd,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),
        AROS_LHAQUAD(double, z, D2, D3),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 11, MathIeeeDoubBas)

/*  FUNCTION
        Calculate the sum of two IEEE double precision numbers

    INPUTS

    RESULT
        sum of y and z

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is too large or too small for ffp format

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD Res;
    QUAD Mant1, Mant2;
    QUAD Qtmp1, Qtmp2;
    LONG Shift;
    QUAD Exponent;
    LONG Ltmp1, Ltmp2;
    QUAD * Qy = (QUAD *)&y;
    QUAD * Qz = (QUAD *)&z;
    double * DRes = (double *)&Res;
    
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    
    AND64C(Qtmp1, *Qy, IEEEDPExponent_Mask_Hi, IEEEDPExponent_Mask_Lo);
    AND64C(Qtmp2, *Qz, IEEEDPExponent_Mask_Hi, IEEEDPExponent_Mask_Lo);
    SHRU32(Ltmp1, Qtmp1, 52);
    SHRU32(Ltmp2, Qtmp2, 52);
    Shift = Ltmp1 - Ltmp2;
    
    if
    (
           is_neqC(*Qy, 0x0, 0x0) 
        && is_neqC(*Qy, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo) 
    )
    {
        AND64C(Mant1, *Qy, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
        OR64QC(Mant1, 0x00100000, 0x00000000);
    }
    else
    {
        Set_Value64C(Mant1, 0x0, 0x0);
    }
    
    if
    (
           is_neqC(*Qz, 0x0, 0x0) 
        && is_neqC(*Qz, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo)
    )
    {
        /* Mant2 = (*Qz & IEEESPMantisse_Mask) | 0x00800000; */
        AND64C(Mant2, *Qz, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
        OR64QC(Mant2, 0x00100000, 0x00000000);
    }
    else
    {
        Set_Value64C(Mant2, 0x0, 0x0);
    }
    
    if (Shift > 0)
    {
        SHRU64(Mant2, Mant2, Shift);
        AND64C(Exponent, *Qy, IEEEDPExponent_Mask_Hi, IEEEDPExponent_Mask_Lo);
    }
    else
    {
        SHRU64(Mant1, Mant1, (-Shift));
        AND64C(Exponent, *Qz, IEEEDPExponent_Mask_Hi, IEEEDPExponent_Mask_Lo);
    }
    
    // sign(fnum1) == sign(fnum2)
    // simple addition
    // 0.25 <= res < 1
    AND64C(Qtmp1, *Qz, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
    AND64C(Qtmp2, *Qy, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
    if ( is_eq(Qtmp1, Qtmp2))
    {
        AND64C(Res, *Qy, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
        ADD64Q(Mant1, Mant2);
        AND64C(Qtmp1, Mant1, 0x00200000, 0x0);
        if ( is_neqC(Qtmp1, 0x0, 0x0))
        {
            ADD64QC(Exponent, 0x00100000, 0x0);
            SHRU64(Mant1, Mant1, 1);
        }
        AND64QC(Mant1, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    }
    // second case: sign(fnum1) != sign(fnum2)
    // -1 <= res < 1
    else
    {
        //printf("Exponent: %x\n",Exponent);
        if ( is_lessSC(*Qy, 0x0, 0x0))
        {
            SUB64(Mant1, Mant2, Mant1);
        }
        else // fnum2 < 0
        {
            SUB64(Mant1, Mant1, Mant2);
        }
        
        //if the result is below zero
        if (  is_lessSC(Mant1, 0x0, 0x0))
        {
            Set_Value64C(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
            NEG64(Mant1);
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        }
        else
        {
            Set_Value64C(Res, 0x0, 0x0);
        }
        
        if (is_eqC(Mant1, 0x0, 0x0))
        {
            union { QUAD i; double d; } tmp;
            SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            Set_Value64C(Res, 0x0, 0x0);
            tmp.i = Res;
            return tmp.d;
        }
        else
        {
            /* normalize the mantisse */
            AND64C(Qtmp1, Mant1, 0x00100000, 0x0);
            while ( is_eqC(Qtmp1, 0x0, 0x0))
            {
                SHL64(Mant1, Mant1, 1);  //one bit to the left.
                SUB64QC(Exponent, 0x00100000, 0x0);
                AND64C(Qtmp1, Mant1, 0x00100000, 0x0);
            }
            AND64QC(Mant1, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
        } /* else */
        
    } /* else */
    
    if ( is_lessSC(Exponent, 0x0, 0x0))
    {
        SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit); //do not change Negative_Bit!
        OR64QC
        (
            Res, 
            (IEEEDPMantisse_Mask_Hi | IEEEDPExponent_Mask_Hi),
            (IEEEDPMantisse_Mask_Lo | IEEEDPExponent_Mask_Lo)
        );
    }
    OR64Q(Res, Mant1);
    OR64Q(Res, Exponent);
    
    return *DRes;

    AROS_LIBFUNC_EXIT
}
