/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/12/10 14:00:02  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.6  1996/11/08 11:28:01  aros
    All OS function use now Amiga types

    Moved intuition-driver protos to intuition_intern.h

    Revision 1.5  1996/10/24 15:51:18  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/10/04 15:33:43  digulla
    Optimized: Draws now as many consecutive pixels as possible

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

	AROS_LH4(void, DrawImage,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct Image    *, image, A1),
	AROS_LHA(LONG             , leftOffset, D0),
	AROS_LHA(LONG             , topOffset, D1),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG   apen;
    ULONG   drmd;
    WORD    x, y, d, plane;
    WORD    xoff, yoff;
    UWORD * bits[24];
    UWORD   bitmask;
    UWORD   shift;
    UWORD   offset;
    ULONG   pen;
    ULONG   lastPen;
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

	yoff = image->TopEdge + topOffset;

	for (y=0; y < image->Height; y++, yoff++)
	{
	    bitmask = START_BITMASK;

	    xoff = image->LeftEdge + leftOffset;

	    for (x=0; x < image->Width; x++, xoff++)
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

		if (!x)
		{
		    lastPen = pen;
		    Move (rp, xoff, yoff);
		}

		if (pen != lastPen)
		{
		    SetAPen (rp, lastPen);
		    Draw (rp, xoff, yoff);
		    lastPen = pen;
		}

		bitmask >>= 1;

		if (!bitmask)
		{
		    bitmask = START_BITMASK;
		    offset ++;
		}
	    }

	    SetAPen (rp, pen);
	    Draw (rp, xoff-1, yoff);

	    if (bitmask != START_BITMASK)
		offset ++;
	}

    }

    /* Restore RastPort */
    SetAPen (rp, apen);
    SetDrMd (rp, drmd);

    AROS_LIBFUNC_EXIT
} /* DrawImage */
