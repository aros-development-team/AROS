/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function ScalerDiv()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(UWORD, ScalerDiv,

/*  SYNOPSIS */
	AROS_LHA(UWORD, factor, D0),
	AROS_LHA(UWORD, numerator, D1),
	AROS_LHA(UWORD, denominator, D2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 114, Graphics)

/*  FUNCTION
	Use this to precalculate the width/height of the destination
	bitmap. As factor give the width/height of the original bitmap
	that is to be scaled via ScaleBitMap(), as numerator give
	the value you will write into bsa_XSrcFactor/bsa_YSrcFactor
	and as denominator the value of bsa_XDestFactor/bsa_YDestFactor.

    INPUTS
	factor      - a number in the range of 0..16383
	numerator   - a number in the range of 1..16383
	denominator - a number in the range of 1..16383

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	BitMapScale()

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  ULONG res;
  if (0 == factor)
    return 0;
  else
    res = (ULONG)((ULONG)factor * (ULONG)numerator) / denominator;

  if (0 == res)
    return 1;

  if (((factor * numerator) % denominator) >= ((denominator + 1) >> 1))
    res++;

  return res;

  AROS_LIBFUNC_EXIT
} /* ScalerDiv */
