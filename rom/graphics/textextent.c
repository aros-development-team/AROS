/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Calculate the size a text needs in a specific rastport.
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <proto/graphics.h>

	AROS_LH4(void, TextExtent,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort   *, rp, A1),
	AROS_LHA(CONST_STRPTR       , string, A0),
	AROS_LHA(ULONG              , count, D0),
	AROS_LHA(struct TextExtent *, textExtent, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 115, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct TextFont *tf = rp->Font;
    
    textExtent->te_Width = TextLength(rp, string, count);
    textExtent->te_Height = tf->tf_YSize;
    textExtent->te_Extent.MinY = -tf->tf_Baseline;
    textExtent->te_Extent.MaxY = textExtent->te_Height - 1 - tf->tf_Baseline;

    /* MinX/MaxX can be a bit more complicated if there are kerning/space tables */
    
    if ((tf->tf_Flags & FPF_PROPORTIONAL) || tf->tf_CharKern || tf->tf_CharSpace)
    {
    	WORD  idx;
	WORD  defaultidx = NUMCHARS(tf) - 1; /* Last glyph is the default glyph */
    	WORD  x, x2;
	UBYTE c;
	
    	textExtent->te_Extent.MinX = 0;
    	textExtent->te_Extent.MaxX = 0;
	x = 0;
	
	if (count)
	{
	    while(count--)
	    {
		c = *string++;

		if ( c < tf->tf_LoChar || c > tf->tf_HiChar)
		{
		    idx = defaultidx;
		}
		else
		{
		    idx = c - tf->tf_LoChar;
		}

		#define CHECK_MINMAX(x) \
	    	    if ((x) < textExtent->te_Extent.MinX) textExtent->te_Extent.MinX = (x); \
		    if ((x) > textExtent->te_Extent.MaxX) textExtent->te_Extent.MaxX = (x);

		x += ((WORD *)tf->tf_CharKern)[idx];
		CHECK_MINMAX(x);

		x2 = x + ( ( ((ULONG *)tf->tf_CharLoc)[idx] ) & 0xFFFF);
		CHECK_MINMAX(x2);

		x += ((WORD *)tf->tf_CharSpace)[idx];
		CHECK_MINMAX(x);

		x += rp->TxSpacing;
		CHECK_MINMAX(x);
		    	    	    
	    } /* while(count--) */
	    
	    textExtent->te_Extent.MaxX--;
	    
	} /* if (count) */
	
    } /* if ((tf->tf_Flags & FPF_PROPORTIONAL) || tf->tf_CharKern || tf->tf_CharSpace) */
    else
    {
    	/* Normal non-proportional Font */
    	textExtent->te_Extent.MinX = 0;
    	textExtent->te_Extent.MaxX = textExtent->te_Width - 1;
    }
    
    if (rp->AlgoStyle & FSF_BOLD)
    {
    	textExtent->te_Extent.MaxX += tf->tf_BoldSmear;
    }
    
    if (rp->AlgoStyle & FSF_ITALIC)
    {
    	/*  ######            ######
	**  ##  ##            ##  ##
	**  ##  ##           ##  ##
	**  ##  ##  ===>     ##  ##
	**  ##  ##	    ##  ##
	**..##..##..      ..##..##..
	**  ##  ##         ##  ##
	**  ######         ######
	*/
	
	textExtent->te_Extent.MaxX += tf->tf_Baseline / 2;
	textExtent->te_Extent.MinX -= (tf->tf_YSize - tf->tf_Baseline) / 2;
    }
    
    AROS_LIBFUNC_EXIT
    
} /* TextExtent */
