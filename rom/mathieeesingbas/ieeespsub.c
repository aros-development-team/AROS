/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*
   FUNCTION
        Subtract two ieeesp numbers
        x = y-z;

    RESULT

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:
        x = y - z = y + (-z).

    HISTORY
*/

AROS_LH2(float, IEEESPSub,
    AROS_LHA(float, y, D0),
    AROS_LHA(float, z, D1),
    struct LibHeader *, MathIeeeSingBasBase, 12, Mathieeesingbas
)
{
    AROS_LIBFUNC_INIT
    
    return IEEESPAdd(y, z ^ IEEESPSign_Mask);

    AROS_LIBFUNC_EXIT
}
