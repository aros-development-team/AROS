/*
    (C) 1995-98 AROS - The Amiga Research OS
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
	- this function *might* not use the blitter

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

  driver_BltClear (memBlock, bytecount, flags, GfxBase);

  AROS_LIBFUNC_EXIT
} /* BltClear */
