/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD1(LONG, IEEEDPTst,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 8, MathIeeeDoubBas)

/*  FUNCTION
	Compare a IEEE double precision floting point number against zero.

    INPUTS

    RESULT
	+1 : y > 0.0
	 0 : y = 0.0
	-1 : y < 0.0

	Flags:
	  zero	   : result is zero
	  negative : result is negative
	  overflow : 0

    BUGS

    INTERNALS
        ALGORITHM:
	Sign is negative: return -1
	y == 0		: return 0
	Otherwise	: return 1

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD * Qy = (QUAD *)&y;
    
    /* y is negative */
    if (is_lessSC(*Qy, 0x0, 0x0) /* y < 0 */)
    {
        SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return -1;
    }
    
    /* fnum1 is zero */
    if (is_eqC(*Qy, 0x0, 0x0) /* y == 0 */)
    {
        SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return 0;
    }
    
    /* fnum1 is positive */
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    
    return 1;

    AROS_LIBFUNC_EXIT
}
