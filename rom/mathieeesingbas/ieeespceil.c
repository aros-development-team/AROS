/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*
    FUNCTION
        Calculate the least integer ieeesp-number greater than or equal to
        y

    RESULT
        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        @Math.Floor@

    INTERNALS
      ALGORITHM:
         Ceil(y) = - Floor(-y)

    HISTORY
*/

AROS_LH1(float, IEEESPCeil,
    AROS_LHA(float, y, D0),
    struct LibHeader *, MathIeeeSingBasBase, 16, Mathieeesingbas
)
{
    AROS_LIBFUNC_INIT
    
    if (y == 0x7f880000) return y;
    
    /* Ceil(y) = -Floor(-y); */
    y = IEEESPFloor(y ^ IEEESPSign_Mask);
    if (y == 0) return 0;
    else        return (y ^ IEEESPSign_Mask);

    AROS_LIBFUNC_EXIT
}
