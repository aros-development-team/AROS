/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1998/02/01 21:47:20  bergers
    Use float instead of LONG when calling these functions now.
    A define in mathffp_intern.h does the trick.

    Revision 1.6  1997/09/16 23:00:46  bergers
    Added the missing AROS_LIBFUNC_INITs and EXITs

    Revision 1.5  1997/07/21 20:56:40  bergers
    *** empty log message ***

    Revision 1.3  1997/07/03 18:35:06  bergers
    Replaced multiple addition by multiplication -> faster now
                Improved overflow handling

    Revision 1.2  1997/06/25 21:36:44  bergers
    *** empty log message ***

    Revision 1.1  1997/05/30 20:50:59  aros
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

        AROS_LH2(float, SPMul,

/*  SYNOPSIS */
        AROS_LHA(float, fnum1, D1),
        AROS_LHA(float, fnum2, D0),

/*  LOCATION */
        struct MathBase *, MathBase, 13, Mathffp)

/*  FUNCTION
        Multiply two ffp numbers
        fnum = fnum1 * fnum2;

    INPUTS
        fnum1  - ffp number
        fnum2  - ffp number

    RESULT

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  char Exponent = ((char) fnum1 & FFPExponent_Mask) +
                  ((char) fnum2 & FFPExponent_Mask) - 0x41;
  ULONG Mant1H = ( (ULONG) (fnum1 & FFPMantisse_Mask)) >> 20;
  ULONG Mant2H = ( (ULONG) (fnum2 & FFPMantisse_Mask)) >> 20;
  ULONG Mant1L = (((ULONG) (fnum1 & FFPMantisse_Mask)) >> 8) & 0x00000fff;
  ULONG Mant2L = (((ULONG) (fnum2 & FFPMantisse_Mask)) >> 8) & 0x00000fff;
  LONG Res;

  Res  =  (Mant1H * Mant2H) <<  8;
  Res += ((Mant1H * Mant2L) >>  4);
  Res += ((Mant1L * Mant2H) >>  4);
  Res += ((Mant1L * Mant2L) >> 16);

  /* Bit 32 is not set */
  if ((LONG)Res > 0)
    Res <<= 1; /* rotate the mantisse by one bit to the left */
  else
    Exponent ++;


  if (0 == Res)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  Res |= ((fnum1 & FFPSign_Mask) ^ (fnum2 & FFPSign_Mask) );

  /* overflow? */
  if ((char) Exponent < 0 || (char) Exponent == 0x7f)
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (Res | (FFPMantisse_Mask + FFPExponent_Mask));
  }

  Res |= Exponent;

  if ((char) Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return Res;
AROS_LIBFUNC_EXIT
} /* SPMul */
