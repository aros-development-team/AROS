/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeedoubtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeedoubtrans_intern.h"


QUAD intern_IEEEDPLd(struct MathIeeeDoubTransBase * MathIeeeDoubTransBase,
                     QUAD fnum)
{
QUAD tmp, Res, Mask = 0;
ULONG i = 0;

  /* argument = 0.5 -> return -1.0;*/
  if (is_eqC(fnum, 0x3fe00000, 0x0))
  {
    Set_Value64C(Res, 0xbff00000, 0x0);
    return Res;
  }

  Set_Value64C(Res, 0x0, 0x0);

  while (i <= 52)
  {
    Set_Value64(tmp, fnum);
    AND64QC(tmp, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    OR64QC(tmp, 0x00100000, 0x0);
    /* if (sqrtonehalf < fnum) */
    if ( is_greaterC(tmp, 0x0016a09e, 0x667f3bcc) )
    {
      i++;
      fnum = IEEEDPMul(fnum, fnum);
    }
    else
    {
      Set_Value64C(Mask, 0x40000000, 0x0);
      fnum = IEEEDPMul(fnum, fnum);
      ADD64QC(fnum, 0x00100000, 0x0);
      break;
    }
  }

  while (is_neqC(Mask, 0x0, 0x200))
  {
    Set_Value64(tmp, fnum);
    AND64QC(tmp, IEEEDPMantisse_Mask_Hi, IEEEDPMantisse_Mask_Lo);
    OR64QC(tmp, 0x00100000, 0x0);
    if ( is_greaterC(tmp, 0x0016a09e, 0x667f3bcc) )
      /* fnum = fnum*fnum */
      fnum = IEEEDPMul(fnum, fnum);
    else
    {
      OR64Q(Res, Mask);
      /* fnum = fnum*fnum * 2 */
      fnum = IEEEDPMul(fnum, fnum);
      ADD64QC(fnum, 0x00100000, 0x0);
    }
    SHRU64(Mask, Mask, 1);
  }

  /* for precision */

  if ( (Get_Low32of64(Res) & 0x400) == 0x400)
    ADD64QC(Res, 0x0, 0x800);

  SHRU64(Res, Res, 11);
  OR64QC(Res, IEEEDPSign_Mask_Hi, IEEEDPSign_Mask_Lo);
  SHL32(tmp, (0x3fe - i) , 52);
  OR64Q(Res, tmp);
  return Res;
} /* intern_IEEEDPLd  */
