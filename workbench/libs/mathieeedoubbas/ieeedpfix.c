/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(LONG, IEEEDPFix,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 5, MathIeeeDoubBas)

/*  FUNCTION
	Convert IEEE double precision floating point number to integer

    INPUTS

    RESULT
	absolute value of y

	Flags:
	  zero	   : result is zero
	  negative : result is negative
	  overflow : ieeedp out of integer-range

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    LONG Res, Shift, tmp;
    QUAD y2;
    QUAD * Qy = (QUAD *)&y;
    
    tmp = Get_High32of64(*Qy) & IEEEDPExponent_Mask_Hi;
    
    if ( tmp > 0x41d00000 )
    {
    if( is_lessSC(*Qy, 0x0, 0x0))
    {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0x80000000;
    }
    else
    {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0x7fffffff;
    }
    }
    
    
    if(
           is_eqC(*Qy, 0x0, 0x0)
        || is_eqC(*Qy,IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo)
    ) /* y=+-0; */
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    Shift = (Get_High32of64(*Qy) & IEEEDPExponent_Mask_Hi) >> 20;
    Shift = 0x433 - Shift;
    tmp = Get_High32of64(*Qy);
    AND64QC(*Qy, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    OR64QC(*Qy, 0x00100000, 0x00000000);
    SHRU64(y2, *Qy , Shift);
    Res = Get_Low32of64(y2);
    
    /* Test for a negative sign  */
    if (tmp < 0) /* y < 0 */
    {
        Res = -Res;
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    
    return Res;

    AROS_LIBFUNC_EXIT
}
