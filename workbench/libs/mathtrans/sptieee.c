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

      AROS_LH1(LONG, SPTieee,

/*  SYNOPSIS */

      AROS_LHA(LONG, fnum, D0),

/*  LOCATION */
      
      struct Library *, MathTransBase, 17, Mathtrans)
      
/*  FUNCTION

       Convert FFP number to single precision ieee number

    INPUTS

       fnum - Motorola fast floting point number

    RESULT

       IEEE Single Precision Floating Point


       flags:
         zero     : result is zero
         negative : result is negative
         overflow : exponent of the ieee-number was out of range for ffp

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
  LONG Res;
  LONG Exponent;
  if (0 == fnum)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  Exponent = (fnum & FFPExponent_Mask) - 0x40 + 126;

  Res = ( Exponent << (30-7) );
  Res |= (((ULONG)fnum & 0x7fffff00) >> 8);

  if ((char) fnum < 0)
  {
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    Res |= IEEESPSign_Mask;
  }

  return Res;
} /* SPTIEEE */
