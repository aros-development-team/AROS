/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1997/09/10 12:21:51  digulla
    Added warning that the two parameters are swapped.

    Revision 1.3  1997/07/21 20:56:40  bergers
    *** empty log message ***

    Revision 1.2  1997/06/25 21:36:44  bergers
    Improved overflow handling

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

	AROS_LH2(LONG, SPDiv,

/*  SYNOPSIS */
	AROS_LHA(LONG, fnum1, D1),
	AROS_LHA(LONG, fnum2, D0),

/*  LOCATION */
	struct MathBase *, MathBase, 14, Mathffp)

/*  FUNCTION
	Divide two ffp numbers
	fnum = fnum2 / fnum1;

    INPUTS
	fnum1  - ffp number
	fnum2  - ffp number

    RESULT

	Flags:
	  zero	   : result is zero
	  negative : result is negative
	  overflow : result is out of range

    NOTES

    EXAMPLE

    BUGS
	The parameters are swapped !

    SEE ALSO


    INTERNALS
      ALGORITHM:
	Check if fnum2 == 0: result = 0;
	Check if fnum1 == 0: result = overflow;
	The further algorithm comes down to a pen & paper division

    HISTORY

******************************************************************************/

{
  LONG Res = 0;
  char Exponent = ((char) fnum2 & FFPExponent_Mask) -
		  ((char) fnum1 & FFPExponent_Mask) + 0x41;

  LONG Mant2 = ((ULONG)fnum2 & FFPMantisse_Mask);
  LONG Mant1 = ((ULONG)fnum1 & FFPMantisse_Mask);
  ULONG Bit_Mask = 0x80000000;

  /* check if the dividend is zero */
  if (0 == fnum2)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  /* check for division by zero */
  if (0 == fnum1)
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0;
  }

  while (Bit_Mask >= 0x40 && Mant2 != 0)
  {
    if (Mant2 - Mant1 >= 0)
    {
      Mant2 -= Mant1;
      Res |= Bit_Mask;

      while (Mant2 > 0)
      {
	Mant2 <<= 1;
	Bit_Mask >>= 1;
      }

      while (Mant1 > 0)
      {
	Mant1 <<=1;
	Bit_Mask <<=1;
      }
    } /* if */
    else
    {
      Mant1 = (ULONG) Mant1 >> 1;
      Bit_Mask >>= 1;
    }
  } /* while */

  /* normalize the mantisse */
  while (Res > 0)
  {
    Res += Res;
    Exponent --;
  }

  if ((char) Res < 0)
    Res += 0x00000100;

  Res &= FFPMantisse_Mask;
  Res |= (Exponent & 0x7f);
  Res |= (fnum1 & FFPSign_Mask) ^ (fnum2 & FFPSign_Mask);

  if ((char) Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);

  if ((char) Exponent < 0)
  {
    SetSR(Overflow_Bit, Zero_Bit | Overflow_Bit | Negative_Bit);
    return(Res | (FFPMantisse_Mask | FFPExponent_Mask));
  }
  return Res;
} /* SPDiv */
