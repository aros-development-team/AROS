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

      AROS_LH1(LONG, SPTanh,

/*  SYNOPSIS */

      AROS_LHA(LONG, fnum1, D0),

/*  LOCATION */

      struct Library *, MathTransBase, 12, Mathtrans)

/*  FUNCTION

      Calculate hyperbolic tangens of the ffp number

    INPUTS

      fnum1 - Motorola fast floating point number

    RESULT

      Motorola fast floating point number


      flags:
        zero     : result is zero
        negative : result is negative
        overflow : (not possible)

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

                ( e^x - e^(-x) )
     tanh(x) =  ----------------
                ( e^x + e^(-x) )

     tanh( |x| > 9 ) = 1

    HISTORY

******************************************************************************/

{
ULONG Res;
LONG tmp;
  tmp = (fnum1 & FFPExponent_Mask) - 0x41;

  if ( tmp >= 3  &&  (fnum1 & FFPMantisse_Mask) >= 0x90000000 )
  /* tanh( x > 9 ) =  1
  ** tanh( x <-9 ) = -1
  */
  {
    return (one | ( fnum1 & FFPSign_Mask ));
  }

  /* tanh(-x) = -tanh(x) */
  Res = SPExp(fnum1 & (FFPMantisse_Mask + FFPExponent_Mask ));
  Res = SPDiv( SPAdd(Res, SPDiv(Res, one)),
               SPAdd(Res, SPDiv(Res, one) | FFPSign_Mask )  );

  /* Result is zero */
  if (0 == Res )
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  /* Argument is negative -> result is negative */
  if ( (char) fnum1 < 0)
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (Res | FFPSign_Mask );
  }

  return Res;
} /* SPTanh */
