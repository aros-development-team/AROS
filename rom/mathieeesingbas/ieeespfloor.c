/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang:
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesp_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(LONG, IEEESPFloor,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 15, Mathieeesingbas)

/*  FUNCTION
        Calculate the largest integer ieeesp-number less than or equal to
        fnum


    INPUTS
        y  - IEEE single precision floating point

    RESULT
             IEEE single precision floating point

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : 0 (???)

    NOTES

    EXAMPLE
       floor(10.5) = 10
       floor(0.5)  = 0
       floor(-0.5) = -1
       floor(-10.5)= -11

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  LONG Mask = 0x80000000;

  if (0x7f880000 == y)
        return y;

  if ((y & IEEESPExponent_Mask)  < 0x3f800000)
  {
    if (y < 0) /* negative sign? */
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0xbf800000; /* -1 */
    }
    else
    {
      SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0;
    }
  }
  /* |fnum| >= 1 */
  Mask >>= ((y & IEEESPExponent_Mask) >> 23) - 0x77;

  /* y is negative */
  if (y < 0)
  /* is there anything behind the decimal dot? */
    if (0 != (y & (~Mask)) )
    {
      y    = IEEESPAdd(y, 0xbf800000 ); /* fnum = fnum -1; */
      Mask = 0x80000000;
      Mask >>= ((y & IEEESPExponent_Mask) >> 23) - 0x77;
    }

  if(y < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return y & Mask;
AROS_LIBFUNC_EXIT
} /* IEEESPFloor */

