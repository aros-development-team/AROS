/*
    Copyright (C) 1995-2017, The AROS Development Team. All rights reserved.
*/

#include "mathieeedoubbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LHQUAD2(double, IEEEDPSub,

/*  SYNOPSIS */
        AROS_LHAQUAD(double, y, D0, D1),
        AROS_LHAQUAD(double, z, D2, D3),

/*  LOCATION */
        struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 12, MathIeeeDoubBas)

/*  FUNCTION
        Subtracts two IEEE double precision floating point numbers.

    INPUTS
        y - minuend.
        z - subtrahend.

    RESULT
        x - difference.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IEEEDPNeg()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    QUAD * Qz = (QUAD *)&z;
    
    XOR64QC(*Qz, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo );
    return IEEEDPAdd(y,z);

    AROS_LIBFUNC_EXIT
}
