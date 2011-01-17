/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*
    FUNCTION
        Subtracts two IEEE double precision numbers

    RESULT
       <code>
       +1 : y > z
        0 : y = z
       -1 : y < z

        Flags:
          zero     : y = z
          negative : y < z
          overflow : 0
       </code>

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LHQUAD2(double, IEEEDPSub,
    AROS_LHAQUAD(double, y, D0, D1),
    AROS_LHAQUAD(double, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 12, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT

    QUAD * Qz = (QUAD *)&z;
    
    XOR64QC(*Qz, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo );
    return IEEEDPAdd(y,z);

    AROS_LIBFUNC_EXIT
}
