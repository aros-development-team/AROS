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

    RESULT
        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IEEEDPFloor()

    INTERNALS
      ALGORITHM:
         Ceil(y) = - Floor(-y)

    HISTORY

******************************************************************************/
{
AROS_LIBFUNC_INIT

  if (is_eqC(y,0,0))
  {
    SetSR(Zero_Bit, Negative_Bit|Overflow_Bit|Zero_Bit);
    return y;
  }

  XOR64QC(y, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
  /* Ceil(y) = -Floor(-y); */
  y = IEEEDPFloor(y);
  if (is_eqC(y,0x0,0x0))
  {
    QUAD tmp;
    Set_Value64C(tmp, 0, 0);
    return tmp;
  }
  else
  {
    XOR64QC(y, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo );
    return y;
  }

AROS_LIBFUNC_EXIT
} /* IEEEDPCeil */

