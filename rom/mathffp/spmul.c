/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

        AROS_LH2(LONG, SPMul,

/*  SYNOPSIS */
        AROS_LHA(LONG, fnum1, D1),
        AROS_LHA(LONG, fnum2, D0),

/*  LOCATION */
        struct MathffpBase *, MathBase, 13, Mathffp)

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
/*
    ALGORITHM
        Description will follow!

*/
{
  char Exponent = ((char) fnum1 & FFPExponent_Mask) +
                  ((char) fnum2 & FFPExponent_Mask) -1 - 0x40;
  ULONG Mant1 = fnum1 & FFPMantisse_Mask;
  ULONG Mant2 = fnum2 & FFPMantisse_Mask;
  ULONG Testbit = 0x80000000;
  LONG Res = 0;
  int Count = 1;

  while (Mant1 != 0)
  {
    if (Testbit & Mant1)
    {
      Res += (Mant2 >> Count);
      Mant1 -= Testbit;
    }

    Testbit >>= 1;
    Count ++;
  }

  //Bit 32 is not set
  if ((LONG)Res > 0)
    Res <<= 1; //rotate the mantisse by one bit to the left
  else
    Exponent ++;


  if (0 == Res)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  Res |= ((fnum1 & FFPSign_Mask) ^ (fnum2 & FFPSign_Mask) );

  if ((char) Exponent < 0)
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (Res | (FFPMantisse_Mask + FFPExponent_Mask));
  }

  Res |= Exponent;


  if ((char) Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return Res;
} /* SPMul */

