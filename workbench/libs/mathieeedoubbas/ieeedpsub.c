/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <aros/libcall.h>
#include <proto/mathieeedoubbas.h>
#include <proto/exec.h>
#include <exec/types.h>
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
	Subtracts two IEEE double precision numbers

    INPUTS
	y  - IEEE double precision floating point
	z  - IEEE double precision floating point

    RESULT
       +1 : y > z
	0 : y = z
       -1 : y < z


	Flags:
	  zero	   : y = z
	  negative : y < z
	  overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
AROS_LIBFUNC_INIT

  return 0ULL;

AROS_LIBFUNC_EXIT
} /* IEEEDPSub */

