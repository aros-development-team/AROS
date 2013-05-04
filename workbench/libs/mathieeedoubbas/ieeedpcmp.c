/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD2(LONG, IEEEDPCmp,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),
        AROS_LHAQUAD(double, z, D2, D3),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 7, MathIeeeDoubBas)

/*  FUNCTION
	Compares two IEEE double precision numbers

    INPUTS

    RESULT
       +1 : y > z
	0 : y = z
       -1 : y < z

	Flags:
	  zero	   : y = z
	  negative : y < z
	  overflow : 0

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    QUAD * Qy = (QUAD *)&y;
    QUAD * Qz = (QUAD *)&z;
    
    if (is_eq(*Qy,*Qz))
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }
    
    if (is_lessSC(*Qy,0,0) /* y < 0 */ && is_lessSC(z,0,0) /* z < 0 */)
    {
        NEG64(*Qy);
        NEG64(*Qz);
        if (is_greater(*Qy,*Qz) /* -y > -z */ )
        {
            SetSR(0,  Zero_Bit | Negative_Bit | Overflow_Bit);
            return 1;
        }
        else
        {
            SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
            return -1;
        }
    }
    
    if ( is_less(*Qy,*Qz)  /* (QUAD)y < (QUAD)z */ )
    {
        SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return -1;
    }
    else
    {
        SetSR(0,  Zero_Bit | Negative_Bit | Overflow_Bit);
        return 1;
    }
  
    AROS_LIBFUNC_EXIT
}
