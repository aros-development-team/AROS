/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Revision 1.3  1997/07/03 18:35:06  bergers
    Replaced multiple addition by multiplication -> faster now

    Desc:
    Lang:
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingbas_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH2(float, IEEESPMul,

/*  SYNOPSIS */
        AROS_LHA(float, y, D0),
        AROS_LHA(float, z, D1),

/*  LOCATION */
        struct MathIeeeSingBasBase *, MathIeeeSingBasBase, 13, Mathieeesingbas)

/*  FUNCTION
        Multiply two IEEE single precision numbers
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

    HISTORY

******************************************************************************/

{
AROS_LIBFUNC_INIT
  ULONG Mant1H = ((y & 0x00fff000) >> 12 ) | 0x00000800;
  ULONG Mant2H = ((z & 0x00fff000) >> 12 ) | 0x00000800;
  ULONG Mant1L = y & 0x00000fff;
  ULONG Mant2L = z & 0x00000fff;
  LONG Res;
  LONG Exponent =((( y & IEEESPExponent_Mask)) +
                  (( z & IEEESPExponent_Mask)) -
                  0x3f800000 );

  if (0 == y || 0 == z)
  {
    SetSR(Zero_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return ((y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask) );
  }

  Res  =  (Mant1H * Mant2H) <<  8;
  Res += ((Mant1H * Mant2L) >>  4);
  Res += ((Mant1L * Mant2H) >>  4);
  Res += ((Mant1L * Mant2L) >> 16);

  /* Bit 32 is not set */
  if ((LONG)Res > 0)
    Res += Res;
  else
    Exponent += 0x00800000;

  /* Correction for precision */
  if ((char) Res < 0)
    Res += 0x100;

  Res >>= 8;
  Res &= IEEESPMantisse_Mask;

  Res |= ((y & IEEESPSign_Mask) ^ (z & IEEESPSign_Mask) );

  if ( Exponent < 0)
  {
    SetSR(Overflow_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);
    return (0x7f800000 | (Res & IEEESPSign_Mask) );
  }

  Res |= Exponent;

  if ( Res < 0)
    SetSR(Negative_Bit, Zero_Bit | Negative_Bit | Overflow_Bit);

  return Res;
AROS_LIBFUNC_EXIT
} /* IEEESPMul */
