/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tell how many characters will fit into a box.
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/graphics.h>

	AROS_LH8(ULONG, TextFit,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort   *, rp, A1),
	AROS_LHA(CONST_STRPTR       , string, A0),
	AROS_LHA(ULONG              , strLen, D0),
	AROS_LHA(struct TextExtent *, textExtent, A2),
	AROS_LHA(struct TextExtent *, constrainingExtent, A3),
	AROS_LHA(LONG               , strDirection, D1),
	AROS_LHA(ULONG              , constrainingBitWidth, D2),
	AROS_LHA(ULONG              , constrainingBitHeight, D3),

/*  LOCATION */
	struct GfxBase *, GfxBase, 116, Graphics)

/*  FUNCTION
	Tries to fill the given space with as many characters of the
	font in rp as possible and returns that number.

    INPUTS
	rp - Use the settings in this RastPort (eg. Font)
	string - Use this string
	strLen - The length of the string
	textExtent - The size actually occupied will be returned here
	constrainingExtent - If non-NULL, the routine will use the
		dimensions of the box described here
	strDirection - In which is the next character. Must be either 1
		or -1. If it is -1, then string must point to the end (the
		first character to check) of the text to fit (this is for
		checking text which runs from right to left).
	constrainingBitWidth - If constrainingExtent is NULL, then this
		is the width of the bounding box.
	constrainingBitHeight - If constrainingExtent is NULL, then this
		is the height of the bounding box.

    RESULT
	The number of characters which fit in the bounding box.
	If any characters fit in the bounding box, then textExtent will
	tell how large the minimal bounding box for the string is.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	TextLength()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct TextFont *tf = rp->Font;
    ULONG   	     retval = 0;
    
    if (strLen && (constrainingBitHeight >= tf->tf_YSize))
    {
   	BOOL ok = TRUE;
	
	textExtent->te_Extent.MinX 	= 0;
	textExtent->te_Extent.MinY 	= -tf->tf_Baseline;
    	textExtent->te_Extent.MaxX 	= 0;
	textExtent->te_Extent.MaxY 	= tf->tf_YSize - tf->tf_Baseline - 1;
    	textExtent->te_Width	= 0;
	textExtent->te_Height 	= tf->tf_YSize;
	
	if (constrainingExtent)
	{
	    if ((constrainingExtent->te_Extent.MinY > textExtent->te_Extent.MinY) || 
	    	(constrainingExtent->te_Extent.MaxY < textExtent->te_Extent.MaxY) ||
	    	(constrainingExtent->te_Height < textExtent->te_Height))
	    {
	    	ok = FALSE;
	    }    
	}
	
	if (ok)
	{
	    while(strLen--)
	    {
    	    	struct TextExtent char_extent;
	    	WORD 	    	  newwidth, newminx, newmaxx, minx, maxx;
		
	    	TextExtent(rp, string, 1, &char_extent);
	    	string += strDirection;
		
		newwidth = textExtent->te_Width + char_extent.te_Width;
		minx = textExtent->te_Width + char_extent.te_Extent.MinX;
		maxx = textExtent->te_Width + char_extent.te_Extent.MaxX;
		
		newminx = (minx < textExtent->te_Extent.MinX) ? minx : textExtent->te_Extent.MinX;
		newmaxx = (maxx > textExtent->te_Extent.MaxX) ? maxx : textExtent->te_Extent.MaxX;
		
		if (newmaxx - newminx + 1 > constrainingBitWidth) break;
		
		if (constrainingExtent)
		{
		    if (constrainingExtent->te_Extent.MinX > newminx) break;
		    if (constrainingExtent->te_Extent.MaxX < newmaxx) break;
		    if (constrainingExtent->te_Width < newwidth) break;
		}
				
		textExtent->te_Width = newwidth;
		textExtent->te_Extent.MinX = newminx;
		textExtent->te_Extent.MaxX = newmaxx;
		
    	    	retval++;		
		
	    } /* while(strLen--) */
	    
	} /* if (ok) */
	
    } /* if (strLen && (constrainingBitHeight >= tf->tf_YSize)) */

    if (retval == 0)
    {
    	textExtent->te_Width = 0;
	textExtent->te_Height = 0;
	textExtent->te_Extent.MinX = 0;
	textExtent->te_Extent.MinY = 0;
	textExtent->te_Extent.MaxX = 0;
	textExtent->te_Extent.MaxY = 0;
    }
    
    return retval;
    
    AROS_LIBFUNC_EXIT
    
} /* TextFit */
