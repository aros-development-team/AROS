/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/graphics.h>

/*****************************************************************************

    NAME */

#include <graphics/gfx.h>

    AROS_LH4I(VOID, InitBitMap,

/*  SYNOPSIS */

	AROS_LHA(struct BitMap *, bm,     A0),
	AROS_LHA(BYTE           , depth,  A1),
	AROS_LHA(UWORD          , width,  D0),
	AROS_LHA(UWORD          , height, D1),

/*  LOCATION */

	struct Library *, GfxBase, 65, Graphics)

/*  FUNCTION

    Initialize BitMap structure. A bitmap MUST be initialized before it's
    used in any (other) graphics library function.

    INPUTS

    bm     --  pointer to BitMap structure
    depth  --  number of bitplanes
    width  --  width in pixels of this bitmap
    height --  height in pixels of this bitmap

    RESULT

    NOTES

    The Planes[] is not affected and must be set up the caller.

    EXAMPLE

    BUGS

    SEE ALSO

    <graphics/gfx.h>

    INTERNALS

    HISTORY

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    bm->Depth = depth;
    bm->Rows  = height;
    bm->Flags = 0;
    bm->pad   = 0;
    bm->BytesPerRow = ((width + 15) >> 3) & ~0x1;

    AROS_LIBFUNC_EXIT
} /* InitBitMap */
