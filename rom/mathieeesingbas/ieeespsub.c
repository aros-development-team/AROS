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

        AROS_LH2(float, IEEESPSub,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),
        AROS_LHA(float, z, D1),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 12, Mathieeesingbas)

/*  FUNCTION
        Subtract two ieeesp numbers
        x = y-z;

    INPUTS
        y  - IEEE single precision floating point
        z  - IEEE single precision floating point

    RESULT

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:
        x = y - z = y + (-z).

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  return IEEESPAdd(y, z ^ IEEESPSign_Mask);
AROS_LIBFUNC_EXIT
} /* IEEESPSub */

