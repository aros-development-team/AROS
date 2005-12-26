/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function BltClear()
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, BltClear,

/*  SYNOPSIS */
	AROS_LHA(void *, memBlock, A1),
	AROS_LHA(ULONG , bytecount, D0),
	AROS_LHA(ULONG , flags, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 50, Graphics)

/*  FUNCTION
	Use the blitter for clearing a block of Chip-Ram.

    INPUTS
	memBlock  - pointer to beginning of memory to be cleared
	flags	  - set bit 0 to force function to wait until
		    the blitter - if used - is done
		    set bit 1 for row/bytesperrow - mode
	bytecount - if bit 1 is set to 1: bytecount contains an even number
					  of bytes to clear
		    if bit 1 is set to 0: low 16 bits are taken as number of
					  bytes per row and upper 16 bits
					  are taken as number of rows.

    RESULT
	A cleared block of Chip-Ram.

    NOTES
        THIS FUNCTION IS DEPRECATED except if you want to simply clear
        some memory.

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() Animate() graphics/rastport.h graphics/gels.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  ULONG count, end;

  if (0 != (flags & 2) )
    /* use row/bytesperrow */
    bytecount = (bytecount & 0xFFFF) * (bytecount >> 16);

  /* we have an even number of BYTES to clear here */
  /* but use LONGS for clearing the block */
  count = 0;
  end = bytecount >> 2;
  while(count < end)
    ((ULONG *)memBlock)[count++] = 0;
  /* see whether we had an odd number of WORDS */
  if (0 != (bytecount & 2))
    ((UWORD *)memBlock)[(count * 2)] = 0;

  AROS_LIBFUNC_EXIT
} /* BltClear */
