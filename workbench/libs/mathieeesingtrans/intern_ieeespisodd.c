/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <libraries/mathieeesp.h>
#include <aros/libcall.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeesingtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathieeesingtrans_intern.h"


LONG intern_IEEESPisodd(LONG fnum)
{
  LONG Exponent = ((fnum & IEEESPExponent_Mask) >> 23) - 0x7f;
  LONG Mask = (0x00800000 >> Exponent);

  if ((fnum & Mask) != 0)
    return TRUE;
  else
    return FALSE;
} /* intern_IEEESPisodd  */
