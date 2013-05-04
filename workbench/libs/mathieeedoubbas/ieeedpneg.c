/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(double, IEEEDPNeg,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 10, MathIeeeDoubBas)

/*  FUNCTION
	Switch the sign of the given IEEE double precision
        floating point number

    INPUTS

    RESULT
	-y

	Flags:
	  zero	   : result is zero
	  negative : result is negative
	  overflow : 0

    BUGS

    INTERNALS
        ALGORITHM:
	Flip the sign-bit (even for zeroes).

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD * Qy = (QUAD *)&y;
    
    /* change the sign-bit */
    XOR64QC(*Qy, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
    
    if
    (
           is_eqC(*Qy, 0x0, 0x0 ) 
        || is_eqC(*Qy, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo)
    )
    {
        SetSR( Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return y;
    }
    
    /* if (y < 0) */
    if(is_lessSC(*Qy, 0x0, 0x0) )
    {
        /* result is negative */
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    }
    else
    {
        /* result is positive */
        SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    }
    
    return y;

    AROS_LIBFUNC_EXIT
}
