/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/09/18 14:43:42  digulla
    Made DrawImage work
    After OpenWindow() one *must* call Wait() to allow X11 to draw for now.

    Revision 1.2  1996/08/29 13:33:30  digulla
    Moved common code from driver to Intuition
    More docs

    Revision 1.1  1996/08/23 17:28:18  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"
#include <clib/graphics_protos.h>

#define DEBUG 1
#include <aros/debug.h>
#include <clib/aros_protos.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <intuition/intuition.h>
	#include <clib/intuition_protos.h>

	__AROS_LH4(void, DrawImage,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A0),
	__AROS_LHA(struct Image    *, image, A1),
	__AROS_LHA(long             , leftOffset, D0),
	__AROS_LHA(long             , topOffset, D1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 19, Intuition)

/*  FUNCTION
	Draw an image.

    INPUTS
	rp - The RastPort to render into
	image - The image to render
	leftOffset, topOffset - Where to place the image.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG   apen;
    ULONG   drmd;
    int     x, y, d, plane;
    UWORD * bits[24];
    UWORD   bitmask;
    UWORD   shift;
    UWORD   offset;
    ULONG   pen;
#define START_BITMASK	0x8000L

    /* Store important variables of the RastPort */
    apen = GetAPen (rp);
    drmd = GetDrMd (rp);

    /* Change RastPort to the mode I need */
    SetDrMd (rp, JAM1);

    /* For all borders... */
    for ( ; image; image=image->NextImage)
    {
	/* Use x to store size of one image plane */
	x = ((image->Width + 15) >> 4) * image->Height;
	y = 0;
	shift = image->PlanePick;

	for (d=0; d < image->Depth; d++)
	{
	    while (!(shift & 1) )
	    {
		bits[y ++] = NULL;
		shift >>= 1;
	    }

	    bits[y ++] = image->ImageData + d * x;
	    shift >>= 1;
	}

	offset	= 0;

	for (y=0; y < image->Height; y++)
	{
	    bitmask = START_BITMASK;

	    for (x=0; x < image->Width; x++)
	    {
		pen = image->PlaneOnOff;
		shift = 1;
		plane = 0;

		for (d=0; d < image->Depth; d++)
		{
		    while (!bits[plane])
		    {
			plane ++;
			shift <<= 1;
		    }

		    pen |= (bits[plane][offset] & bitmask) ? shift : 0;

		    plane ++;
		    shift <<= 1;
		}

/* kprintf (" x=%2d y=%2d   offset=%3d bitmask=%04x bits[]=%04x pen=%d\n"
    , x
    , y
    , offset
    , bitmask
    , bits[0][offset]
    , pen
); */

		SetAPen (rp, pen);
		WritePixel (rp
		    , x + image->LeftEdge + leftOffset
		    , y + image->TopEdge + topOffset
		);

		bitmask >>= 1;

		if (!bitmask)
		{
		    bitmask = START_BITMASK;
		    offset ++;
		}
	    }

	    if (bitmask != START_BITMASK)
		offset ++;
	}

    }

    /* Restore RastPort */
    SetAPen (rp, apen);
    SetDrMd (rp, drmd);

    __AROS_FUNC_EXIT
} /* DrawImage */
