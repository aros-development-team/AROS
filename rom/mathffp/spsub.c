/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*
    FUNCTION
        Subtract two floating point numbers
        fnum = fnum2 - fnum1;

    RESULT
	FFP number

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SPAdd()

    INTERNALS
      ALGORITHM:
        fnum = fnum2 + (-fnum1).

    HISTORY
*/

AROS_LH2(float, SPSub,
    AROS_LHA(float, fnum1, D0),
    AROS_LHA(float, fnum2, D1),
    struct LibHeader *, MathBase, 12, Mathffp
)
{
    AROS_LIBFUNC_INIT
    
    ULONG r = SPAdd(fnum2, fnum1 ^ FFPSign_Mask);
    kprintf("%x - %x = %x\n",fnum2,fnum1,r);
    return r;

    AROS_LIBFUNC_EXIT
}
