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

        AROS_LH2(LONG, IEEESPMul,

/*  SYNOPSIS */
        AROS_LHA(LONG, y, D0),
        AROS_LHA(LONG, z, D1),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeespbas)

/*  FUNCTION
        Multiply two ffp numbers
        res = y * z;

    INPUTS
        y  - IEEE single precision floating point
        z  - IEEE single precision floating point

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
      ALGORITHM
          Pen and paper algorithm of multiplication

    HISTORY

******************************************************************************/

{

  ULONG Mant1, Mant2;
  ULONG Testbit = 0x80000000;
  LONG Res = 0;
  int Count = 1;
  LONG Exponent =((( y & IEEESPExponent_Mask)) +
                  (( z & IEEESPExponent_Mask)) -
                  0x3f800000 );

  if (y != 0)
    Mant1 = ((y & IEEESPMantisse_Mask) | 0x00800000 )<< 8;
  else
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return ((y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask) );
  }

  if (z != 0)
    Mant2 = ((z & IEEESPMantisse_Mask) | 0x00800000 )<< 8;
  else
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return ((y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask) );
  }

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

  /* Correction for precision */
  if ((char) Res < 0)
    Res += 0x100;

  /* Bit 32 is not set */
  if ((LONG)Res > 0)
    Res >>= 7;
  else
  {
    Exponent += 0x00800000;
    Res >>= 8;
  }

  Res &= IEEESPMantisse_Mask;

  Res |= ((y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask) );

  if ( Exponent < 0)
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return 0x7f800000;
  }

  Res |= Exponent;

  if ( Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return Res;
} /* IEEESPMul */

