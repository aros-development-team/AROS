/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function SetRPAttrsA()
    Lang: english
*/

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
    BOOL    	    havedriverdata = FALSE;
    
    while ((tag = NextTagItem ((const struct TagItem **)&tstate)))
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
	    
	    case RPTAG_FgColor:
	    case RPTAG_BgColor:
	    {
	    	IPTR attr;
		
	    	if (tag->ti_Tag == RPTAG_FgColor)
		{
		    attr = aHidd_GC_Foreground;
		    RP_FGCOLOR(rp) = (ULONG)tag->ti_Data;
		}
		else
		{
		    attr = aHidd_GC_Background;
		    RP_BGCOLOR(rp) = (ULONG)tag->ti_Data;
		}
		
		if (!rp->BitMap) break;
		if (!IS_HIDD_BM(rp->BitMap)) break;
		
	    	if (!havedriverdata)
		{
		    havedriverdata = OBTAIN_DRIVERDATA(rp, GfxBase);
		}
		
		if (havedriverdata)
		{
		    struct TagItem col_tags[] =
		    {
		    	{ attr, 0   },
			{ TAG_DONE  }
		    };
		    HIDDT_Color col;
		    ULONG rgb = (ULONG)tag->ti_Data;
		    
		    /* HIDDT_ColComp are 16 Bit */
		    col.alpha	= (HIDDT_ColComp)((rgb >> 16) & 0x0000FF00);
		    col.red	= (HIDDT_ColComp)((rgb >> 8) & 0x0000FF00);
		    col.green	= (HIDDT_ColComp)(rgb & 0x0000FF00);
		    col.blue	= (HIDDT_ColComp)((rgb << 8) & 0x0000FF00);

		    col_tags[0].ti_Data = HIDD_BM_MapColor(HIDD_BM_OBJ(rp->BitMap), &col);
    	    	    OOP_SetAttrs(RP_DRIVERDATA(rp)->dd_GC, col_tags);		    		    		    
		}		
		break;
		
	    } /**/

	    case RPTAG_ClipRectangle:
	    	if (!havedriverdata)
		{
		    havedriverdata = OBTAIN_DRIVERDATA(rp, GfxBase);
		}
	    	
		if (havedriverdata)
		{
		    if (tag->ti_Data)
		    {
		    	RP_DRIVERDATA(rp)->dd_ClipRectangle = *(struct Rectangle *)tag->ti_Data;
			RP_DRIVERDATA(rp)->dd_ClipRectangleFlags |= RPCRF_VALID;
		    }
		    else
		    {
		    	RP_DRIVERDATA(rp)->dd_ClipRectangleFlags &= ~RPCRF_VALID;
		    }
		}
		break;
		
	    case RPTAG_ClipRectangleFlags:
	    	if (!havedriverdata)
		{
		    havedriverdata = OBTAIN_DRIVERDATA(rp, GfxBase);
		}
	    	
		if (havedriverdata)
		{
		    RP_DRIVERDATA(rp)->dd_ClipRectangleFlags &= ~(RPCRF_RELRIGHT | RPCRF_RELBOTTOM);
		    RP_DRIVERDATA(rp)->dd_ClipRectangleFlags |= (tag->ti_Data & (RPCRF_RELRIGHT | RPCRF_RELBOTTOM));
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

    if (havedriverdata)
    {
    	RELEASE_DRIVERDATA(rp, GfxBase);
    }
    
    AROS_LIBFUNC_EXIT
} /* SetRPAttrsA */
