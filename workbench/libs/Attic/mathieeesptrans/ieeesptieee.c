/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
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

      AROS_LH1(LONG, IEEESPTieee,

/*  SYNOPSIS */
      AROS_LHA(LONG, y, D0),

/*  LOCATION */
      struct Library *, MathIeeeSingTransBase, 17, Mathieeesingtrans)


/*  FUNCTION
        Convert IEEE single to IEEE single
        It just returns the input parameter.

    INPUTS
        y  - IEEE single precision floating point number

    RESULT
        IEEE single precision floting point number

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/
{
AROS_LIBFUNC_INIT
  return y;
AROS_LIBFUNC_EXIT
} /* IEEESPTieee */

