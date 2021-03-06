/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD2(double, IEEEDPDiv,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),
        AROS_LHAQUAD(double, z, D2, D3),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 14, MathIeeeDoubBas)

/*  FUNCTION
        Divides two IEEE double precision numbers.

    INPUTS
        y - dividend.
        z - divisor.

    RESULT
        x - quotient.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return y / z;

    AROS_LIBFUNC_EXIT
}
