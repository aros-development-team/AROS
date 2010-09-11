/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*
    FUNCTION
        Compare a ffp-number against zero.

    RESULT
	<code>
        +1 : fnum > 0.0
         0 : fnum = 0.0
        -1 : fnum < 0.0

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
        Sign is negative: return -1
        fnum == 0       : return 0
        Otherwise       : return 1

    HISTORY
*/

AROS_LH1(LONG, SPTst,
    AROS_LHA(float, fnum, D1),
    struct LibHeader *, MathBase, 8, Mathffp
)
{
    AROS_LIBFUNC_INIT
    /* fnum1 is negative */
    if ((char) fnum < 0)
    {
        SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return -1;
    }
    
    /* fnum1 is zero */
    if (0 == fnum)
    {
        SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
        return 0;
    }
    
    /* fnum1 is positive */
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    
    return 1;

    AROS_LIBFUNC_EXIT
}
