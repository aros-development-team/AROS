/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Render an image.
    Lang: english
*/
#include "intuition_intern.h"
#include <proto/graphics.h>
#include <aros/macros.h>

#undef DEBUG
#define DEBUG 0
#	include <aros/debug.h>
#include <proto/arossupport.h>

#define USE_BLTBITMAPRASTPORT 1
#define USE_FASTPLANEPICK0    1

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
    WORD    x, y, d;
    UWORD   shift;
    ULONG   planeonoff, planepick;

#if !USE_BLTBITMAPRASTPORT

    ULONG   apen;
    ULONG   drmd;
    ULONG   lastPen = 0;
    ULONG   pen = 0;
    UWORD   offset;
    UWORD   bitmask;
    UWORD * bits[8];
    WORD    xoff, yoff, plane;

    #define START_BITMASK	0x8000L

    /* Store important variables of the RastPort */
    apen = GetAPen (rp);
    drmd = GetDrMd (rp);

    /* Change RastPort to the mode I need */
    SetDrMd (rp, JAM1);
    
#endif
    
    
    /* For all images... */
    for ( ; image; image=image->NextImage)
    {
/*	kprintf("*** Drawing Image %x. Next Image = %x\n widht = %d  height = %d  depth = %d  planepick = %d  planeonoff = %d\n",
		image,image->NextImage,
		image->Width,image->Height,image->Depth,image->PlanePick,image->PlaneOnOff);*/
	

	planepick  = image->PlanePick;
        planeonoff = image->PlaneOnOff & ~planepick;

#if USE_FASTPLANEPICK0

	if (planepick == 0)
	{
	    SetAPen(rp, planeonoff);
	    RectFill(rp, leftOffset + image->LeftEdge,
	    		 topOffset  + image->TopEdge,
			 leftOffset + image->LeftEdge + image->Width  - 1,
			 topOffset  + image->TopEdge  + image->Height - 1);
	    
	    continue;
	}

#endif
	
	/* Use x to store size of one image plane */
	x = ((image->Width + 15) >> 4) * image->Height;
	y = 0;

	shift = 1;

#if USE_BLTBITMAPRASTPORT
	{
	    struct BitMap bitmap;
	    
	    /* The "8" (instead of image->Depth) seems to be correct,
	       as for example DOpus uses prop gadget knob images with
	       a depth of 0 (planepick 0, planeonoff color) */
	       
	    InitBitMap(&bitmap, 8, image->Width, image->Height);
	    
	    for(d = 0; d < 8; d++)
	    {
	        if (planepick & shift)
		{
		     bitmap.Planes[d] = (PLANEPTR)(image->ImageData + (y++) * x);
		} else {
		     bitmap.Planes[d] = (planeonoff & shift) ? (PLANEPTR)-1 : NULL;
		}
		shift <<= 1;
	    }

	    BltBitMapRastPort(&bitmap,
	    		      0, 0,
	    		      rp,
			      leftOffset + image->LeftEdge, topOffset + image->TopEdge,
			      image->Width, image->Height,
			      192);
	}
	
#else
	
	
	for(d = 0; d < image->Depth;d++)
	{
	    bits[d] = (planepick & shift) ? image->ImageData + (y++) * x : NULL;
	    shift <<= 1;	
	}

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

#endif
	
    } /* for ( ; image; image=image->NextImage) */

#if !USE_BLTBITMAPRASTPORT

    /* Restore RastPort */
    SetAPen (rp, apen);
    SetDrMd (rp, drmd);

#endif

    AROS_LIBFUNC_EXIT
} /* DrawImage */
