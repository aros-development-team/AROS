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

      AROS_LHQUAD1(double, IEEEDPCeil,

/*  SYNOPSIS */
      AROS_LHAQUAD(double, y, D0, D1),

/*  LOCATION */
      struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 16, MathIeeeDoubBas)

/*  FUNCTION
	Calculates the ceil-value of a IEEE double precision number

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
} /* IEEEDPCeil */

