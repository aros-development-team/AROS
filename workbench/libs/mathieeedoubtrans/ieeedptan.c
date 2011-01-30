/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*
    FUNCTION
      Calculate the tangens of the given IEEE double precision number
      where y represents an angle in radians.

    RESULT
      result - IEEE double precision floating point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LHQUAD1(double, IEEEDPTan,
    AROS_LHAQUAD(double, y, D0, D1),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 8, MathIeeeDoubTrans
)
{
    AROS_LIBFUNC_INIT
    QUAD sn, cs, Res;
    
    cs = IEEEDPCos(y);
    sn = IEEEDPSin(y);
    if (is_eqC(cs, 0, 0))
    {
        Set_Value64C(Res, IEEEDPPInfty_Hi, IEEEDPPInfty_Lo);
        return Res;
    }
    else
    {
        return IEEEDPDiv(sn, cs);
    }

    AROS_LIBFUNC_EXIT
}

