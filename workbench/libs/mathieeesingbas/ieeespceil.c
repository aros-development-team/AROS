/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(float, IEEESPCeil,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),

/*  LOCATION */
        struct LibHeader *, MathIeeeSingBasBase, 16, Mathieeesingbas)

/*  FUNCTION
        Calculate the least integer ieeesp-number
        greater than or equal to y

    INPUTS

    RESULT
        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    BUGS

    SEE ALSO
        IEEESPFloor()

    INTERNALS
        ALGORITHM:
        Ceil(y) = - Floor(-y)

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    if (y == 0x7f880000) return y;
    
    /* Ceil(y) = -Floor(-y); */
    y = IEEESPFloor(y ^ IEEESPSign_Mask);
    if (y == 0) return 0;
    else        return (y ^ IEEESPSign_Mask);

    AROS_LIBFUNC_EXIT
}
