/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <libraries/mathffp.h>
#include <aros/libcall.h>
#include <proto/mathffp.h>
#include <proto/mathtrans.h>
#include <proto/exec.h>
#include <exec/types.h>
#include "mathtrans_intern.h"

LONG intern_SPisodd(ULONG fnum)
{
  char Exponent = ((fnum & FFPExponent_Mask)) - 0x41;
  ULONG Mask = (0x80000000 >> Exponent);

  if ((fnum & Mask) != 0)
    return TRUE;
  else
    return FALSE;

} /* intern_SPisodd  */
