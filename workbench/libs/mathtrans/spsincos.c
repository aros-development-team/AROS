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

      AROS_LH2(LONG, SPSincos,

/*  SYNOPSIS */

      AROS_LHA(IPTR*, pfnum2, D1),
      AROS_LHA(LONG , fnum1 , D0),

/*  LOCATION */

      struct Library *, MathTransBase, 9, Mathtrans)

/*  FUNCTION

      Calculate the cosine and the sine of the given ffp-number
      fnum1 that represents an angle in radians. The function
      returns the sine of that number as a result and puts
      the cosine of that number into *pfnum2 which must represent
      a valid pointer to a ffp-number.

    INPUTS

      pfnum2  - pointer to a Motorola fast floating point number
      fnum1   - Motorola fast floating point number

    RESULT

      *pfnum2       - Motorola fast floating point number
      direct result - Motorola fast floating point number


    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
  * pfnum2 = SPCos(fnum1);
  return SPSin(fnum1);
} /* SPSincos */

