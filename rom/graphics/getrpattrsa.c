/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetRPAttrsA()
    Lang: english
*/
#include <graphics/rpattr.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(void, GetRPAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp  , A0),
	AROS_LHA(struct TagItem  *, tags, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 174, Graphics)

/*  FUNCTION

	Read the current settings of a RastPort into variables.
	The ti_Tag field specifies the attribute to read and the
	ti_Data field points to an address where to store the result.
	All results are stored as LONGs (32 bits)!

	Available tags:

		RPTAG_Font		Font for Text()
		RPTAG_APen		Primary rendering pen
		RPTAG_BPen		Secondary rendering pen
		RPTAG_DrMd		Drawing mode (graphics/rastport.h)
		RPTAG_OutlinePen	Area Outline pen
		RPTAG_WriteMask		Bit Mask for writing
		RPTAG_MaxPen		Maximum oen to render (see SetMaxPen())
		RPTAG_DrawBounds	Determine the area that will be redered
					into by rendering commands. Can be used
					to optimize window refresh. Pass a pointer
					to a rectangle in the ti_Data field. On
					return the rectangle's MinX will be
					greater than its MaxX if there are no
					active cliprects.

    INPUTS
	rp   = pointer to a RastPort structure
	tags = pointer to a taglist specifying the attributes to read and
	       the addresses to store the results

    RESULT

    NOTES

    EXAMPLE

    BUGS
	RPTAG_SoftStyle not supported, yet.

    SEE ALSO
	SetRPAttrsA() GetAPen() GetBPen() GetOutLinePen() GetWriteMask()
	graphics/rpattr.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct TagItem * tag, *tstate = tags;
    ULONG MaxPen, z;

    while ((tag = NextTagItem ((const struct TagItem **)&tstate)))
    {
	switch(tag->ti_Tag)
	{
	    case RPTAG_Font :
            	*((IPTR *)tag->ti_Data) = (IPTR)rp->Font;
	    	break;

	    case RPTAG_APen :
            	*((IPTR *)tag->ti_Data) = (IPTR)GetAPen(rp);
	    	break;

	    case RPTAG_BPen :
            	*((IPTR *)tag->ti_Data) = (IPTR)GetBPen(rp);
	    	break;

	    case RPTAG_DrMd :
            	*((IPTR *)tag->ti_Data) = (IPTR)GetDrMd(rp);
	    	break;

	    case RPTAG_OutlinePen :
            	*((IPTR *)tag->ti_Data) = (IPTR)GetOutlinePen(rp);
	    	break;

	    case RPTAG_WriteMask :
            	*((IPTR *)tag->ti_Data) = (IPTR)rp->Mask;
	    	break;

	    case RPTAG_MaxPen :
        	MaxPen = 0x01;
        	z = (LONG)rp->Mask;
        	if (0 == z)
        	    MaxPen = 0x100;
        	else
        	    while (z != 0)
        	    {
        	    	z >>= 1;
        	    	MaxPen <<= 1;
        	    }
        	*((IPTR *)tag->ti_Data) = MaxPen;
	    	break;

	    case RPTAG_DrawBounds :
	    	((struct Rectangle *)tag->ti_Data)->MinX = 0;
	    	((struct Rectangle *)tag->ti_Data)->MinY = 0;
	    	((struct Rectangle *)tag->ti_Data)->MaxX = 0;
	    	((struct Rectangle *)tag->ti_Data)->MaxY = 0;
	    	break;

	} /* switch(tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem ((const struct TagItem **)&tstate))) */

    AROS_LIBFUNC_EXIT
} /* GetRPAttrsA */
