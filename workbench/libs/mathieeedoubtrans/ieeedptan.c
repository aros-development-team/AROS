/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: $
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

AROS_LH1(double, IEEEDPTan,
    AROS_LHA(double, y, D0),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 8, MathIeeeDoubTrans
)
{
    AROS_LIBFUNC_INIT
    double sn, cs;
    
    cs = IEEEDPCos(y);
    sn = IEEEDPSin(y);
    if (cs == 0)
    {
        if (sn > 0)
	{
            return IEEEDPPInfty_Hi;
	}
	else
	{
            return IEEEDPPInfty_Lo;
	}
    }
    else
    {
        return sn/cs;
    }

    AROS_LIBFUNC_EXIT
}

