/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create a new BitMap
    Lang: english
*/
#include <string.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <cybergraphx/cybergraphics.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/gfx.h>
#include <proto/graphics.h>

	AROS_LH5(struct BitMap *, AllocBitMap,

/*  SYNOPSIS */
	AROS_LHA(ULONG          , sizex, D0),
	AROS_LHA(ULONG          , sizey, D1),
	AROS_LHA(ULONG          , depth, D2),
	AROS_LHA(ULONG          , flags, D3),
	AROS_LHA(struct BitMap *, friend_bitmap, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 153, Graphics)

/*  FUNCTION
	Allocates and initializes a bitmap structure. Allocates and
	initializes bitplane data, and sets the bitmap's planes to point to
	it.

    INPUTS
	sizex, sizey - The width and height in pixels
	depth - The depth of the bitmap. A depth of 1 will allocate a
	    bitmap for two colors, a depth of 24 will allocate a bitmap for
	    16 million colors. Pixels with AT LEAST this many bits will be
	    allocated.
	flags - One of these flags:

	    \begin{description}
	    \item{BMF_CLEAR} Fill the bitmap with color 0.

	    \item{BMF_DISPLAYABLE} to specify that this bitmap data should
		be allocated in such a manner that it can be displayed.
		Displayable data has more severe alignment restrictions
		than non-displayable data in some systems.

	    \item{BMF_INTERLEAVED} tells graphics that you would like your
		bitmap to be allocated with one large chunk of display
		memory for all bitplanes. This minimizes color flashing on
		deep displays. If there is not enough contiguous RAM for an
		interleaved bitmap, graphics.library will fall back to a
		non-interleaved one.

	    \item{BMF_MINPLANES} causes graphics to only allocate enough
		space in the bitmap structure for "depth" plane pointers.
		This is for system use and should not be used by
		applications use as it is inefficient, and may waste
		memory.

	    \item{BMF_SPECIALFMT} causes graphics to allocate a bitmap
	    	of a standard CyberGraphX format. The format
		(PIXFMT_????) must be stored in the 8 most significant bits.

	    \end{description}

	friend_bitmap - pointer to another bitmap, or NULL. If this pointer
	    is passed, then the bitmap data will be allocated in
	    the most efficient form for blitting to friend_bitmap.

    RESULT
	A pointer to the new bitmap.

    NOTES
	When allocating using a friend bitmap, it is not safe to assume
	anything about the structure of the bitmap data if that friend
	BitMap might not be a standard amiga bitmap (for instance, if the
	workbench is running on a non-amiga display device, its
	Screen->RastPort->BitMap won't be in standard amiga format. The
	only safe operations to perform on a non-standard BitMap are:

	    \begin{itemize}
	    \item blitting it to another bitmap, which must be either a
		standard Amiga bitmap, or a friend of this bitmap.

	    \item blitting from this bitmap to a friend bitmap or to a
		standard Amiga bitmap.

	    \item attaching it to a rastport and making rendering calls.

	    \end{itemize}

	Good arguments to pass for the friend_bitmap are your window's
	RPort->BitMap, and your screen's RastPort->BitMap. Do NOT pass
	&(screenptr->BitMap)!

	BitMaps not allocated with BMF_DISPLAYABLE may not be used as
	Intuition Custom BitMaps or as RasInfo->BitMaps.  They may be
	blitted to a BMF_DISPLAYABLE BitMap, using one of the BltBitMap()
	family of functions.

    EXAMPLE

    BUGS

    SEE ALSO
	FreeBitMap()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    struct BitMap * nbm;
    ULONG attributes;

    ASSERT_VALID_PTR_OR_NULL(friend_bitmap);

    /*
	If the depth is too large or the bitmap should be displayable or
	there is a friend bitmap and that's not a normal bitmap, then
	call the RTG driver.
    */
    
    if (
	depth > 8
	|| (flags & BMF_DISPLAYABLE)
/*	|| (friend_bitmap && friend_bitmap->Pad != 0) */
	|| (friend_bitmap && friend_bitmap->Flags & BMF_AROS_HIDD)
#warning Should	we also check for BMF_MINPLANES ?
	|| (flags & BMF_SPECIALFMT) /* Cybergfx bitmap */
    )
    {
	nbm = driver_AllocBitMap (sizex
	    , sizey
	    , depth
	    , flags
	    , friend_bitmap
	    , 0
	    , GfxBase
	);
	ASSERT_VALID_PTR(nbm);
    }
    else /* Otherwise init a plain Amiga bitmap */
    {
        if (flags & BMF_CLEAR)
          attributes = MEMF_ANY|MEMF_CLEAR;
        else
          attributes = MEMF_ANY;
	nbm = AllocMem (sizeof (struct BitMap), attributes);

	if (nbm)
	{
	    int plane;

	    nbm->BytesPerRow = ((sizex + 15) >> 4) * 2;
	    nbm->Rows	     = sizey;
	    nbm->Flags	     = flags;
	    nbm->Depth	     = depth;
	    nbm->Pad	     = 0;

	    for (plane=0; plane<depth; plane++)
	    {
		nbm->Planes[plane] = AllocRaster (sizex, sizey);

		if (!nbm->Planes[plane])
		    break;

		if (flags & BMF_CLEAR)
		    memset (nbm->Planes[plane], 0, RASSIZE(sizex,sizey));
	    }

	    if (plane != depth)
	    {
		for (plane=0; plane<depth; plane++)
		    if (nbm->Planes[plane])
			FreeRaster (nbm->Planes[plane], sizex, sizey);

		FreeMem (nbm, sizeof (struct BitMap));

		nbm = 0;
	    }
	}
    }

    return nbm;
    AROS_LIBFUNC_EXIT
} /* AllocBitMap */
