/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1997/09/16 23:00:46  bergers
    Added the missing AROS_LIBFUNC_INITs and EXITs

    Revision 1.2  1997/06/25 21:36:44  bergers
    *** empty log message ***

    Revision 1.1  1997/05/30 20:50:58  aros
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

        AROS_LH1(LONG, SPFix,

/*  SYNOPSIS */
        AROS_LHA(LONG, fnum, D0),

/*  LOCATION */
        struct MathBase *, MathBase, 5, Mathffp)

/*  FUNCTION
        Convert ffp-number to integer

    INPUTS
        fnum   - ffp number

    RESULT
        absolute value of fnum1

        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : ffp out of integer-range

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  LONG Res;
  BYTE Shift;

  if ((fnum & FFPExponent_Mask) > 0x60 )
    if(fnum < 0) /* don`t hurt the SR! */
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0x80000000;    
    }
    else
    {
      SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0x7fffffff;    
    }


  Shift = (fnum & FFPExponent_Mask) - 0x40;
  Res = ((ULONG)(fnum & FFPMantisse_Mask)) >> (32 - Shift);

  if (0 == Res)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  if (0x80000000 == Res)
    return 0x7fffffff;


  /* Test for a negative sign */
  if ((char)fnum < 0)
  {
    Res = -Res;
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
  }

  return Res;
AROS_LIBFUNC_EXIT
} /* SPFix */

