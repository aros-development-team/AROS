/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

        AROS_LH1(LONG, SPFloor,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),

/*  LOCATION */
        struct MathffpBase *, MathBase, 15, Mathffp)

/*  FUNCTION
        Calculate the largest integer ffp-number less than or equal to
        fnum


    INPUTS
        y  - ffp number

    RESULT
             ffp number

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
/*
    ALGORITHM
   The integer part of a ffp number are the left "exponent"-bits
   of the mantisse!
   Therefore:
   Test the exponent for <= 0. This has to be handled separately!
   If the sign is negative then return -1 otherwise return 0.

   Generate a mask of exponent(y) bits starting with bit 31.
   If fnum < 0 then test whether it is already an integer. If not
   then y = y - 1 and generate that mask again. Use the
   mask on the mantisse.
*/
{
  LONG Mask = 0x80000000;

  if (((char)y & FFPExponent_Mask)  <= 0x40)
  {
    if ((char)y < 0)
    {
      SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0x800000C1; // -1
    }
    else
    {
      SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
      return 0;
    }
  }

  // |fnum| >= 1
  Mask >>= ( ((char) y & FFPExponent_Mask) - 0x41);
  Mask |= FFPSign_Mask | FFPExponent_Mask;

  // fnum is negative
  if ((char) y < 0)
  // is there anything behind the dot?
    if (0 != (y & (~Mask)) )
    {
      Mask = 0x80000000;
      y    = SPAdd(y, 0x800000c1); // fnum = fnum -1;
      Mask >>= ((char) y & FFPExponent_Mask) - 0x41;
      Mask |= FFPSign_Mask | FFPExponent_Mask;
    }

  if((char) y < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return y & Mask;
} /* SPFloor */

