/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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
QUAD tmp, Res, Mask;
ULONG i = 0;

  /* argument = 0.5 -> return -1.0;*/
  if (is_eqC(fnum, 0x3fe00000, 0x0, 0x3fe0000000000000ULL))
  {
    Set_Value64C(Res, 0xbff00000, 0x0, 0xbff0000000000000ULL);
    return Res;
  }

  Set_Value64C(Res, 0x0, 0x0, 0x0ULL);

  while (i <= 52)
  {
    Set_Value64(tmp, fnum);
    AND64QC(tmp, IEEEDPMantisse_Mask_Hi,
                 IEEEDPMantisse_Mask_Lo,
                 IEEEDPMantisse_Mask_64);
    OR64QC(tmp, 0x00100000, 0x0, 0x0010000000000000ULL);
    /* if (sqrtonehalf < fnum) */
    if ( is_greaterC(tmp, 0x0016a09e, 0x667f3bcc, 0x0016a09e667f3bccULL) )
    {
      i++;
      fnum = IEEEDPMul(fnum, fnum);
    }
    else
    {
      Set_Value64C(Mask, 0x40000000, 0x0 ,0x4000000000000000ULL);
      fnum = IEEEDPMul(fnum, fnum);
      ADD64QC(fnum, 0x00100000, 0x0, 0x0010000000000000ULL);
      break;
    }
  }

  while (is_neqC(Mask, 0x0, 0x200, 0x200ULL))
  {
    Set_Value64(tmp, fnum);
    AND64QC(tmp, IEEEDPMantisse_Mask_Hi,
                 IEEEDPMantisse_Mask_Lo,
                 IEEEDPMantisse_Mask_64);
    OR64QC(tmp, 0x00100000, 0x0, 0x0010000000000000ULL);
    if ( is_greaterC(tmp, 0x0016a09e, 0x667f3bcc, 0x0016a09e667f3bccULL) )
      /* fnum = fnum*fnum */
      fnum = IEEEDPMul(fnum, fnum);
    else
    {
      OR64Q(Res, Mask);
      /* fnum = fnum*fnum * 2 */
      fnum = IEEEDPMul(fnum, fnum);
      ADD64QC(fnum, 0x00100000, 0x0, 0x0010000000000000ULL);
    }
    SHRU64(Mask, Mask, 1);
  }

  /* for precision */

  if ( (Get_Low32of64(Res) & 0x400) == 0x400)
    ADD64QC(Res, 0x0, 0x800, 0x800ULL);

  SHRU64(Res, Res, 11);
  OR64QC(Res, IEEEDPSign_Mask_Hi,
              IEEEDPSign_Mask_Lo,
              IEEEDPSign_Mask_64 );
  SHL32(tmp, (0x3fe - i) , 52);
  OR64Q(Res, tmp);
  return Res;
} /* intern_IEEEDPLd  */
