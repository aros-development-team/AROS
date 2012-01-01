/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function SetRPAttrsA()
    Lang: english
*/

#include <aros/debug.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
#include <utility/tagitem.h>
#include <proto/graphics.h>

	AROS_LH2(void, SetRPAttrsA,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A0),
	AROS_LHA(struct TagItem  *, tags, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 173, Graphics)

/*  FUNCTION
	Modify rastport with values from a taglist.

    INPUTS
	rp   - RastPort
	tags - see below

    TAGS
	RPTAG_Font (struct TextFont *)        - Font for Text()
	RPTAG_APen (UBYTE)                    - Primary rendering pen
	RPTAG_BPen (UBYTE)                    - Secondary rendering pen
	RPTAG_DrMd (UBYTE)                    - Drawing mode (graphics/rastport.h)
	RPTAG_OutlinePen (UBYTE)              - Area Outline pen
	RPTAG_WriteMask (ULONG)               - Bit mask for writing

	The following tags are compatible with MorphOS and AmigaOSv4 (V51) :

	RPTAG_FgColor (ULONG)                 - Primary rendering color in A8R8G8B8 format.
		                                Only working on hicolor/truecolor bitmaps/screens.
	RPTAG_BgColor (ULONG)                 - Secondary rendering color in A8R8G8B8 format.
		    	    	    	        Only working on hicolor/truecolor bitmaps/screens.
	RPTAG_RemapColorFonts (BOOL)          - Automatically remap colorfonts to their color
						on hicolor/truecolor screens.

	AROS-specific extensions

	RPTAG_ClipRectangle (struct Rectangle *) - Clipping rectangle
	RPTAG_ClipRectangleFlags (LONG)       - RPCRF_RELRIGHT | RPCRF_RELBOTTOM (see graphics/rpattrs.h)

    RESULT

    NOTES
    	Setting one of RPTAG_ClipRectangle or RPTAG_ClipRectangleFlags allocates internal extra data
    	for the RastPort. After finishing using this RastPort, you need to manually deallocate
    	the extra data using FreeVec(rp->RP_Extra).

    EXAMPLE

    BUGS

    SEE ALSO
	GetRPAttrsA(), graphics/rpattr.h

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = tags;
    struct gfx_driverdata *driverdata;
    
    while ((tag = NextTagItem (&tstate)))
    {
	switch (tag->ti_Tag)
	{
	    case RPTAG_Font:
		SetFont (rp, (struct TextFont *)(tag->ti_Data));
		break;

	    case RPTAG_APen:
		SetAPen (rp, tag->ti_Data);
		break;

	    case RPTAG_BPen:
		SetBPen (rp, tag->ti_Data);
		break;

	    case RPTAG_DrMd:
		SetDrMd (rp, tag->ti_Data);
		break;

	    case RPTAG_OutlinePen:
		SetOutlinePen (rp, tag->ti_Data);
		break;

	    case RPTAG_WriteMask:
		SetWriteMask (rp, tag->ti_Data);
		break;

	    case RPTAG_MaxPen:
		break;

	    case RPTAG_DrawBounds:
		break;

	    case RPTAG_PenMode:
	    	D(bug("[SetRPAttrs] RastPort 0x%p, PenMode set to %ld\n", rp, tag->ti_Data));
	    	if (tag->ti_Data)
		    rp->Flags &= ~RPF_NO_PENS;
		else
		    rp->Flags |= RPF_NO_PENS;
		break;

	    case RPTAG_FgColor:
	    case RPTAG_BgColor:
	    	D(bug("[SetRPAttrs] RastPort 0x%p, setting %sColor to 0x%08lX\n", rp, (tag->ti_Tag == RPTAG_FgColor) ? "Fg" : "Bg", tag->ti_Data));

		if (rp->BitMap && IS_HIDD_BM(rp->BitMap))
		{
		    /* Map ARGB8888 color value to bitmap's format */
		    HIDDT_Color col;
		    HIDDT_Pixel pixval;
		    ULONG rgb = (ULONG)tag->ti_Data;

		    /* HIDDT_ColComp are 16 Bit */
		    col.alpha	= (HIDDT_ColComp)((rgb >> 16) & 0x0000FF00);
		    col.red	= (HIDDT_ColComp)((rgb >> 8) & 0x0000FF00);
		    col.green	= (HIDDT_ColComp)(rgb & 0x0000FF00);
		    col.blue	= (HIDDT_ColComp)((rgb << 8) & 0x0000FF00);

		    pixval = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);

	    	    if (tag->ti_Tag == RPTAG_FgColor)
		    	RP_FGCOLOR(rp) = pixval;
		    else
		    	RP_BGCOLOR(rp) = pixval;
		}
		break;

	    case RPTAG_ClipRectangle:
	    	driverdata = AllocDriverData(rp, tag->ti_Data, GfxBase);
	    	if (driverdata)
	    	{
	    	    if (tag->ti_Data)
	    	    {
		    	driverdata->dd_ClipRectangle = *(struct Rectangle *)tag->ti_Data;
			driverdata->dd_ClipRectangleFlags |= RPCRF_VALID;
		    }
		    else
		    {
		    	driverdata->dd_ClipRectangleFlags &= ~RPCRF_VALID;
		    }
		}
		break;

	    case RPTAG_ClipRectangleFlags:
	    	driverdata = AllocDriverData(rp, TRUE, GfxBase);
		if (driverdata)
		{
		    driverdata->dd_ClipRectangleFlags &= ~(RPCRF_RELRIGHT | RPCRF_RELBOTTOM);
		    driverdata->dd_ClipRectangleFlags |= (tag->ti_Data & (RPCRF_RELRIGHT | RPCRF_RELBOTTOM));
		}
	    	break;

	    case RPTAG_RemapColorFonts:
	    	if (tag->ti_Data)
		{
		    rp->Flags |= RPF_REMAP_COLORFONTS;
		}
		else
		{
		    rp->Flags &= ~RPF_REMAP_COLORFONTS;
		}
		break;
		
	} /* switch (tag) */
	
    } /* while (tag) */

    AROS_LIBFUNC_EXIT
} /* SetRPAttrsA */
