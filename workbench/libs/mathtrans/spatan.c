/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/


#include <libraries/mathffp.h>
#include <aros/libcall.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathtrans_intern.h"


/*****************************************************************************

    NAME */

      AROS_LH1(float, SPAtan,

/*  SYNOPSIS */

      AROS_LHA(float, fnum1 , D0),

/*  LOCATION */

      struct MathTransBase *, MathTransBase, 5, MathTrans)

/*  FUNCTION

      Calculates the angle of a given number representing the tangent
      of that angle. The angle will be in radians.

    INPUTS

      fnum1   - Motorola fast floating point number

    RESULT

      Motorola fast floating point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  return 0;
AROS_LIBFUNC_EXIT
} /* SPAtan */
