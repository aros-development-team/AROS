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

      AROS_LHQUAD2(LONG, IEEEDPCmp,

/*  SYNOPSIS */
      AROS_LHAQUAD(QUAD, y, D0, D1),
      AROS_LHAQUAD(QUAD, z, D2, D3),

/*  LOCATION */
      struct MathIeeeDoubBasBase *, MathIeeeDoubBasBase, 7, Mathieeedoubbas)

/*  FUNCTION
        Compares two IEEE double precision numbers

    INPUTS
        y  - IEEE double precision floating point
        z  - IEEE double precision floating point

    RESULT
       +1 : y > z
        0 : y = z
       -1 : y < z


        Flags:
          zero     : y = z
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
  if (is_eq(y,z))
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  if (is_lessSC(y,0,0,0ULL) /* y < 0 */ && is_lessSC(z,0,0, 0) /* z < 0 */)
  {
    NEG64(y);
    NEG64(z);
    if (is_greater(y,z) /* -y > -z */ )
    {
      SetSR(0,  Zero_Bit | Negative_Bit | Overflow_Bit);
      return 1;
    }
    else
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return -1;
    }
  }

  if ( is_less(y,z)  /* (QUAD)y < (QUAD)z */ )
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return -1;
  }
  else
  {
    SetSR(0,  Zero_Bit | Negative_Bit | Overflow_Bit);
    return 1;
  }
AROS_LIBFUNC_EXIT
} /* IEEEDPCmp */

