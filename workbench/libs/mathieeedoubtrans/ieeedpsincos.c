/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeedoubtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeedoubtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LH2(double, IEEEDPSincos,

/*  SYNOPSIS */

      AROS_LHA(double *, z, A0),
      AROS_LHA(double  , y, D0),

/*  LOCATION */

      struct Library *, MathIeeeDoubTransBase, 9, Mathieeedoubtrans)

/*  FUNCTION

      Calculate the cosine and the sine of the given IEEE double
      precision number where y represents an angle in radians. The 
      function returns the sine of that number as a result and puts
      the cosine of that number into *z which must represent
      a valid pointer to a IEEE double precision number.

    INPUTS

      z  - pointer to an IEEE double precision floating point number
      y  - IEEE double precision floting point number

    RESULT

      *z            - IEEE double precision floating point number
      direct result - IEEE double precision floating point number


    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  *z = IEEEDPCos(y);
  return IEEEDPSin(y);
AROS_LIBFUNC_EXIT
} /* DPSincos */

