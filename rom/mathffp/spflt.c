/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1997/09/16 23:00:46  bergers
    Added the missing AROS_LIBFUNC_INITs and EXITs

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

        AROS_LH1(LONG, SPFlt,

/*  SYNOPSIS */
        AROS_LHA(LONG, inum, D0),

/*  LOCATION */
        struct MathBase *, MathBase, 6, Mathffp)

/*  FUNCTION


    INPUTS
        inum  - signed integer number

    RESULT


        Flags:
          zero     : result is zero
          negative : result is negative
          overflow : ffp is not exactly the integer

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO


    INTERNALS

    HISTORY

******************************************************************************/
/*
    ALGORITHM
        Return zero for x == 0.
        If x < 0 set the sign-bit and calculate the absolute value
        of x.
        Find out which bit is the highest-set bit. If the number
        of that bit > 24 then the result has the highest bit
        of the mantisse set to one and the exponent equals the
        number of the bit + 2. This is due to the fact that we only
        have 24 bits for the mantisse.
        Otherwise rotate the given integer by
        (32 - (number of highes set bit + 1)) bits to the left and
        calculate the result from that.
*/
{
AROS_LIBFUNC_INIT
  BYTE Exponent = 0;
  LONG TestMask = 0xFFFFFFFF;
  LONG Res = 0;

  if (inum == 0)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  if (inum < 0)
  {
    Res = FFPSign_Mask;
    inum = -inum;
  }
  /* find out which is the number of the highes set bit */
  while (TestMask & inum)
  {
    Exponent ++;
    TestMask <<= 1;
  }

  /* Exponent = number of highest set bit + 1 */

  inum <<= (32 - Exponent);
  if ((char) inum < 0)
    inum +=0x100;
  inum &= FFPMantisse_Mask;

  /* adapt the exponent to the ffp format */
  Exponent += 0x40;
  Res |= inum | Exponent;
  if ((char) Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  if (Exponent > (25 + 0x40))
  {
    Res |= 0x80000000;
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
  }

  return Res;
AROS_LIBFUNC_EXIT
} /* SPFlt */

