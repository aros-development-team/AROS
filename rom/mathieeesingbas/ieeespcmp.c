/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesp_intern.h"

/*****************************************************************************

    NAME */

	AROS_LH2(LONG, IEEESPCmp,

/*  SYNOPSIS */
	AROS_LHA(LONG, y, D0),
	AROS_LHA(LONG, z, D1),

/*  LOCATION */
	struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 7, Mathieeesingbas)

/*  FUNCTION
	Compares two ieeesp numbers

    INPUTS
	y  - IEEE single precision floating point
	z  - IEEE single precision floating point

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
  if (y == z)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  if (y < 0 && z < 0)
    if (-y > -z)
    {
      SetSR(0,  Zero_Bit | Negative_Bit | Overflow_Bit);
      return 1;
    }
    else
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return -1;
    }

  if ((LONG)y < (LONG)z)
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return -1;
  }
  else
  {
    SetSR(0,  Zero_Bit | Negative_Bit | Overflow_Bit);
    return 1;
  }
} /* IEEESPCmp */

