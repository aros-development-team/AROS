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

      AROS_LH1(LONG, SPCos,

/*  SYNOPSIS */

      AROS_LHA(LONG , fnum1 , D0),

/*  LOCATION */

      struct Library *, MathTransBase, 7, Mathtrans)

/*  FUNCTION

      Calculates the cosine of a floating point number representing
      an angle in radians.

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
  return 0;
} /* SPCos */
