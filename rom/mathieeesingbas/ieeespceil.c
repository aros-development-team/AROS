/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(LONG, IEEESPCeil,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 16, Mathieeesingbas)

/*  FUNCTION
        Calculate the least integer ieeesp-number greater than or equal to
        y


    INPUTS
        y  - IEEE single precision

    RESULT


        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IEEESPFloor()

    INTERNALS
      ALGORITHM:
         Ceil(y) = - Floor(-y)

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  if (y == 0x7f880000)
    return y;

  /* Ceil(y) = -Floor(-y); */
  y = IEEESPFloor(y ^ IEEESPSign_Mask);
  if (y == 0)
    return 0;
  else
    return (y ^ IEEESPSign_Mask);
AROS_LIBFUNC_EXIT
} /* IEEESPCeil */

