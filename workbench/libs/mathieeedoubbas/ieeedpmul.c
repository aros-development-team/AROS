/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "mathieeedoubbas_intern.h"

/*
    FUNCTION
	Multiplies two IEEE double precision numbers

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

    SEE ALSO

    INTERNALS

    HISTORY
*/

AROS_LHQUAD2(double, IEEEDPMul,
    AROS_LHAQUAD(double, y, D0, D1),
    AROS_LHAQUAD(double, z, D2, D3),
    struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 13, MathIeeeDoubBas
)
{
    AROS_LIBFUNC_INIT
    
    return 0x0badc0de0badc0deULL;

    AROS_LIBFUNC_EXIT
}
