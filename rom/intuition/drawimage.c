/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Render an image.
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/graphics.h>
#include <aros/macros.h>

#undef DEBUG
#define DEBUG 1
#	include <aros/debug.h>
#include <proto/arossupport.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>

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
    UWORD * bits[8];
    UWORD   bitmask;
    UWORD   shift;
    UWORD   offset;
    ULONG   pen = 0;
    ULONG   planeonoff, planepick;
    ULONG   lastPen = 0;
#define START_BITMASK	0x8000L

    /* Store important variables of the RastPort */
    apen = GetAPen (rp);
    drmd = GetDrMd (rp);

    /* Change RastPort to the mode I need */
    SetDrMd (rp, JAM1);

    planepick  = image->PlanePick;
    planeonoff = image->PlaneOnOff & ~planepick;
    
    /* For all images... */
    for ( ; image; image=image->NextImage)
    {
/*	kprintf("*** Drawing Image %x. Next Image = %x\n widht = %d  height = %d  depth = %d  planepick = %d  planeonoff = %d\n",
		image,image->NextImage,
		image->Width,image->Height,image->Depth,image->PlanePick,image->PlaneOnOff);*/
	
	/* Use x to store size of one image plane */
	x = ((image->Width + 15) >> 4) * image->Height;
	y = 0;

#if 0
	shift = planepick;

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
#else
	shift = 1;
	
	for(d = 0; d < image->Depth;d++)
	{
	    bits[d] = (planepick & shift) ? image->ImageData + (y++) * x : NULL;
	    shift <<= 1;	
	}
#endif

	offset	= 0;

	yoff = image->TopEdge + topOffset;

	for (y=0; y < image->Height; y++, yoff++)
	{
	
	    bitmask = START_BITMASK;

	    xoff = image->LeftEdge + leftOffset;

	    for (x=0; x < image->Width; x++, xoff++)
	    {
		pen = planeonoff;
		shift = 1;
		plane = 0;

#if 0
		for (d=0; d < image->Depth; d++)
		{
		    UWORD dat;
		    
		    while (!bits[plane])
		    {
			plane ++;
			shift <<= 1;
		    }

		    dat = bits[plane][offset];
		    dat = AROS_WORD2BE(dat);
		    
		    pen |= (dat & bitmask) ? shift : 0;

		    plane ++;
		    shift <<= 1;
		}
#else
		for(d = 0; d < image->Depth; d++)
		{
		    if (planepick & shift)
		    {
		        UWORD dat;
			
		        dat = bits[d][offset];
			dat = AROS_WORD2BE(dat);
			
			if (dat & bitmask)
			{
			    pen |= shift;
			}
		    }
		    
		    shift <<= 1;
		}
#endif

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
		
	    } /* for (x=0; x < image->Width; x++, xoff++) */

	    SetAPen (rp, pen);
	    Draw (rp, xoff-1, yoff);

	    if (bitmask != START_BITMASK)
		offset ++;
		
	} /* for (y=0; y < image->Height; y++, yoff++) */
	
    } /* for ( ; image; image=image->NextImage) */

    /* Restore RastPort */
    SetAPen (rp, apen);
    SetDrMd (rp, drmd);

    AROS_LIBFUNC_EXIT
} /* DrawImage */
