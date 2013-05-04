/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include "mathieeedoubbas_intern.h"

/*
    FUNCTION
	Divides two IEEE double precision numbers

    RESULT
       <code>
       +1 : y > z
	0 : y = z
       -1 : y < z

	Flags:
	  zero	   : y = z
	  negative : y < z
	  overflow : 0
       </code>

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO

    INTERNALS

    HISTORY
*/   

AROS_LHQUAD2(double, IEEEDPDiv,
    AROS_LHAQUAD(double, y, D0, D1),
    AROS_LHAQUAD(double, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 14, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT

    union {
        QUAD i;
        double d;
    } Res;
    
#if 0
    QUAD * Qy = (QUAD *)&y;
    QUAD * Qz = (QUAD *)&z;
#endif
    
    Res.i = 0x0badc0de0badc0deULL;

    aros_print_not_implemented("Software IEEEDPDiv");

    return Res.d;

    AROS_LIBFUNC_EXIT
}
