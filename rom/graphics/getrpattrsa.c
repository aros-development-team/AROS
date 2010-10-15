/*
    Copyright � 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetRPAttrsA()
    Lang: english
*/
#include <graphics/rpattr.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include "graphics_intern.h"
#include <proto/utility.h>

#include "gfxfuncsupport.h"

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
	All results are stored as IPTRs!

    INPUTS
	rp   = pointer to a RastPort structure
	tags = pointer to a taglist specifying the attributes to read and
	       the addresses to store the results

    TAGS
	RPTAG_Font (UBYTE)              - Font for Text()
	RPTAG_APen (UBYTE)              - Primary rendering pen
	RPTAG_BPen (UBYTE)              - Secondary rendering pen
	RPTAG_DrMd (UBYTE)              - Drawing mode (graphics/rastport.h)
	RPTAG_OutlinePen (UBYTE)        - Area Outline pen
	RPTAG_WriteMask	(ULONG)	        - Bit Mask for writing
	RPTAG_MaxPen (ULONG)            - Maximum pen to render (see SetMaxPen())

	AROS extensions
	RPTAG_FgColor (ULONG)           - Primary rendering color in A8R8G8B8 format.
		                          Only working on hicolor/truecolor bitmaps/screens.
    	RPTAG_BgColor (ULONG)           - Secondary rendering color in A8R8G8B8 format.
		    	    	          Only working on hicolor/truecolor bitmaps/screens.
    	RPTAG_PatternOriginX (WORD)     - X Origin of fill pattern.
	RPTAG_PatternOriginY (WORD)     - Y Origin of fill pattern.
	RPTAG_ClipRectangle (struct Rectangle *) - Rectangle to clip rendering to. Rectangle will
		                                   be cloned.
	RPTAG_ClipRectangleFlags (LONG) - RPCRF_RELRIGHT | RPCRF_RELBOTTOM (see <graphics/rpattr.h>)
    	RPTAG_RemapColorFonts (BOOL)    - Automatically remap colorfonts to their color
					  on hicolor/truecolor screens.
		
    RESULT

    NOTES
    	RPTAG_ClipRectangle and RPTAG_ClipRectangleFlags must not be
	used on manually inited or cloned rastports. Instead the rastport
	must have been created with CreateRastPort() or CloneRastPort().
	
    EXAMPLE

    BUGS
	RPTAG_SoftStyle not supported, yet.

    SEE ALSO
	SetRPAttrsA(), GetAPen(), GetBPen(), GetOutLinePen(), graphics/rpattr.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tags;
    ULONG   	    MaxPen, z;
    BOOL    	    havedriverdata = FALSE;

    while ((tag = NextTagItem ((const struct Tagitem **)&tstate)))
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

    	    case RPTAG_FgColor:
	    	*((IPTR *)tag->ti_Data) = RP_FGCOLOR(rp);
		break;
		
	    case RPTAG_BgColor:
	    	*((IPTR *)tag->ti_Data) = RP_BGCOLOR(rp);
		break;
		
	    case RPTAG_PatternOriginX:
	    	*((IPTR *)tag->ti_Data) = RP_PATORIGINX(rp);
	    	break;
		
	    case RPTAG_PatternOriginY:
	    	*((IPTR *)tag->ti_Data) = RP_PATORIGINY(rp);
		break;
	    	    
	    case RPTAG_ClipRectangle:
	    	if (!havedriverdata)
		{
		    havedriverdata = OBTAIN_DRIVERDATA(rp, GfxBase);
		}
	    	
		if (havedriverdata)
		{
		    if (RP_DRIVERDATA(rp)->dd_ClipRectangleFlags & RPCRF_VALID)
		    {
			*((struct Rectangle **)tag->ti_Data) = &(RP_DRIVERDATA(rp)->dd_ClipRectangle);
		    }
		    else
		    {
		    	*((struct Rectangle **)tag->ti_Data) = NULL;
		    }
		}
		else
		{
		    *((IPTR *)tag->ti_Data) = 0;
		}
		break;
		
	    case RPTAG_ClipRectangleFlags:
	    	if (!havedriverdata)
		{
		    havedriverdata = OBTAIN_DRIVERDATA(rp, GfxBase);
		}
	    	
		if (havedriverdata)
		{
		    *((IPTR *)tag->ti_Data) = RP_DRIVERDATA(rp)->dd_ClipRectangleFlags;
		}
		else
		{
		    *((IPTR *)tag->ti_Data) = 0;
		}
	    	break;
		
	    case RPTAG_RemapColorFonts:
	    	*((IPTR *)tag->ti_Data) = (rp->Flags & RPF_REMAP_COLORFONTS) ? TRUE : FALSE;
		break;
		
	} /* switch(tag->ti_Tag) */
	
    } /* while ((tag = NextTagItem ((const struct TagItem **)&tstate))) */

    if (havedriverdata)
    {
    	RELEASE_DRIVERDATA(rp, GfxBase);
    }

    AROS_LIBFUNC_EXIT
} /* GetRPAttrsA */
