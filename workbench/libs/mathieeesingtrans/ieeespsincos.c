/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeesingtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LH2(LONG, IEEESPSincos,

/*  SYNOPSIS */

      AROS_LHA(IPTR*, z, A0),
      AROS_LHA(LONG , y, D0),

/*  LOCATION */

      struct Library *, MathIeeeSingTransBase, 9, Mathieeesingtrans)

/*  FUNCTION

      Calculate the cosine and the sine of the given IEEE single
      precision number where y represents an angle in radians. The 
      function returns the sine of that number as a result and puts
      the cosine of that number into *z which must represent
      a valid pointer to a IEEE single precision number.

    INPUTS

      z  - pointer to an IEEE single precision floating point number
      y  - IEEE single precision floting point number

    RESULT

      *z            - IEEE single precision floating point number
      direct result - IEEE single precision floating point number


    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  *z = IEEESPCos(y);
  return IEEESPSin(y);
AROS_LIBFUNC_EXIT
} /* SPSincos */

