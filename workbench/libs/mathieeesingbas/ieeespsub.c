/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(float, IEEESPSub,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),
        AROS_LHA(float, z, D1),

/*  LOCATION */
        struct LibHeader *, MathIeeeSingBasBase, 12, Mathieeesingbas)

/*  FUNCTION
        Subtract two ieeesp numbers
        x = y-z;

    INPUTS

    RESULT
        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    BUGS

    INTERNALS
        ALGORITHM:
        x = y - z = y + (-z).

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    return IEEESPAdd(y, z ^ IEEESPSign_Mask);

    AROS_LIBFUNC_EXIT
}
