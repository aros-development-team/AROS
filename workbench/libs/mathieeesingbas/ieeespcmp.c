/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(LONG, IEEESPCmp,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),
        AROS_LHA(LONG, z, D1),

/*  LOCATION */
        struct LibHeader *, MathIeeeSingBasBase, 7, Mathieeesingbas)

/*  FUNCTION
        Compares two ieeesp numbers

    INPUTS

    RESULT
       +1 : y > z
        0 : y = z
       -1 : y < z

        Flags:
          zero     : y = z
          negative : y < z
          overflow : 0

    BUGS

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (y == z)
    {
        SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
        return 0;
    }

    if (y < 0 && z < 0)
    {
        if (-y > -z)
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

    if ((LONG)y < (LONG)z)
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
