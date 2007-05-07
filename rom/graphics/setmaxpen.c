/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetMaxPen()
    Lang: english
*/
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(void, SetMaxPen,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp    , A0),
	AROS_LHA(ULONG            , maxpen, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 165, Graphics)

/*  FUNCTION
	Set the maximum pen value for a rastport. This will instruct the
	graphics.library that the owner of the rastport will not be rendering
	in any colors whose index is >maxpen. Therefore speed optimizations
	on certain operations are possible and will be done.

	Basically this call sets the rastport mask, if this would improve speed.
	On devices where masking would slow things down (chunky pixels), it will
	be a no-op.

    INPUTS
	rp     = pointer to a valid RastPort structure
	maxpen = longword pen value

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SetWriteMask()

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  BYTE Mask;

  /* maxpen==0 is nonsense */
  if (0 == maxpen)
    return;

  /* calculate the Mask */
  /* maxpen   | Mask   | highest bit
   * 1        | 1      | 0
   * 2..3     | 3      | 1
   * 4..7     | 7      | 2
   * 8..15    | 15     | 3
   * 16..31   | 31     | 4
   * 31..63   | 63     | 5
   * 63..127  | 127    | 6
   * 128..255 | 255    | 7
   */

  /* look for the highest bit */
  Mask = 0x0;
  while ((BYTE)maxpen != 0)
  {
    maxpen >>= 1;
    Mask = (Mask << 1) | 0x01;
  }

  rp->Mask = Mask;

  AROS_LIBFUNC_EXIT
} /* SetMaxPen */
