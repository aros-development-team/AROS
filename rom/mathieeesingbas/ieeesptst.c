/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeesingbas_intern.h"

/*
    FUNCTION
        Compare a ieeesp-number against zero.

    RESULT
	<code>
        +1 : y > 0.0
         0 : y = 0.0
        -1 : y < 0.0

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0
	</code>

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:
	<code>
        Sign is negative: return -1
        y == 0          : return 0
        Otherwise       : return 1
	</code>

    HISTORY
*/

AROS_LH1(LONG, IEEESPTst,
    AROS_LHA(float, y, D0),
    struct LibHeader *, MathIeeeSingBasBase, 8, Mathieeesingbas
)
{
    AROS_LIBFUNC_INIT
    
    /* y is negative */
    if (y < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return -1;
    }
    
    /* fnum1 is zero */
    if (0 == y)
    {
        SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return 0;
    }
    
    /* fnum1 is positive */
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );

    return 1;

    AROS_LIBFUNC_EXIT
}
