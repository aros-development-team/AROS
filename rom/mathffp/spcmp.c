/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1998/02/01 21:47:19  bergers
    Use float instead of LONG when calling these functions now.
    A define in mathffp_intern.h does the trick.

    Revision 1.3  1997/09/16 23:00:46  bergers
    Added the missing AROS_LIBFUNC_INITs and EXITs

    Revision 1.2  1997/06/25 21:36:44  bergers
    *** empty log message ***

    Revision 1.1  1997/05/30 20:50:57  aros
    *** empty log message ***


    Desc:
    Lang: english
*/
#include <libraries/mathffp.h>
#include <aros/libcall.h>
#include <proto/mathffp.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathffp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(LONG, SPCmp,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D0),
        AROS_LHA(float, fnum2, D1),

/*  LOCATION */
        struct MathBase *, MathBase, 7, Mathffp)

/*  FUNCTION
        Compares two ffp numbers

    INPUTS
        fnum1  - ffp number
        fnum2  - ffp number

    RESULT
       +1 : fnum1 > fnum2
        0 : fnum1 = fnum2
       -1 : fnum1 < fnum2


        Flags:
          zero     : fnum2 = fnum1
          negative : fnum2 < fnum1
          overflow : 0

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS
      ALGORITHM:
        1st case:
          fnum1 is negative and fnum2 is positive
            or
          ( exponent(fnum1) < exponent(fnum2) and signs are equal )
          -> fnum1 < fnum2

        2nd case:
          fnum1 is positive and fnum2 is negative
            or
          ( exponent(fnum1) > exponent(fnum2) and signs are equal )
          -> fnum2 > fnum1

        now the signs and exponents must be equal

        3rd case:
          fnum1 == fnum2

        4th case:
          mantisse(fnum1) < mantisse(fnum2)
          -> fnum1 < fnum2

        final case:
          fnum1 > fnum2
    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT

  /* fnum1 is negative and fnum2 is positive
  **  or
  ** exponent of fnum1 is less than the exponent of fnum2
  ** => fnum1 < fnum2
  */
  if ( (char)fnum1 < (char)fnum2 )
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return -1;
  }

  /* fnum1 is positive and fnum2 is negative
  **  or
  ** exponent of fnum1 is greater tban the exponent if fnum2
  ** => fnum1 > fnum2
  */
  if ((char) fnum1 > (char) fnum2 )
  {
    SetSR(0, Zero_Bit | Overflow_Bit | Negative_Bit );
    return 1;
  }

  /*the signs and exponents of fnum1 and fnum2 must now be equal
  **fnum1 == fnum2
  */
  if (fnum1 == fnum2)
  {
    SetSR(Zero_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    return 0;
  }

  /* mantisse(fnum1) < mantisse(fnum2) */
  if (fnum1 < fnum2)
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return -1;
  }

  /* Mantisse(fnum1) > mantisse(fnum2) */
  SetSR(0, Zero_Bit | Negative_Bit | Overflow_Bit);
  return 1;
AROS_LIBFUNC_EXIT
} /* SPCmp */

