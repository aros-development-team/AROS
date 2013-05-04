/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(float, SPSub,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D1),
        AROS_LHA(float, fnum2, D0),

/*  LOCATION */
        struct LibHeader *, MathBase, 12, Mathffp)

/*  FUNCTION
        Subtract two floating point numbers
        fnum = fnum2 - fnum1;

    INPUTS

    RESULT
	FFP number

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    BUGS

    SEE ALSO
	SPAdd()

    INTERNALS
        ALGORITHM:
        fnum = fnum2 + (-fnum1).

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    float r = SPAdd(fnum2, fnum1 ^ FFPSign_Mask);
    D(kprintf("SPSub(%x - %x) = %x\n", fnum2, fnum1, r));
    return r;

    AROS_LIBFUNC_EXIT
}
