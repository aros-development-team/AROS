/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubtrans_intern.h"

/*
    FUNCTION
      Calculate the cosine and the sine of the given IEEE double
      precision number where y represents an angle in radians. The 
      function returns the sine of that number as a result and puts
      the cosine of that number into *z which must represent
      a valid pointer to a IEEE double precision number.

    RESULT
      *z            - IEEE double precision floating point number
      direct result - IEEE double precision floating point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LH2(double, IEEEDPSincos,
    AROS_LHA(double *, z, A0),
    AROS_LHA(double, y, D0),
    struct MathIeeeDoubTransBase *, MathIeeeDoubTransBase, 9, MathIeeeDoubTrans
)
{
    AROS_LIBFUNC_INIT
    
    *z = IEEEDPCos(y);
    return IEEEDPSin(y);

    AROS_LIBFUNC_EXIT
}
