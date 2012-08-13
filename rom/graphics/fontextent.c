/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function FontExtent()
    Lang: English
*/

#include <proto/graphics.h>
#include <graphics/text.h>

#include "graphics_intern.h"

#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))

/*****************************************************************************

    NAME */

	AROS_LH2(void, FontExtent,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont   *, font      , A0),
	AROS_LHA(struct TextExtent *, fontExtent, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 127, Graphics)

/*  FUNCTION

    Fill out a text extent structure with the maximum extent of the
    characters for the font in question.

    INPUTS

    font        --  The font the extent of which to calculate.
    fontExtent  --  TextExtent structure to hold the values.

    RESULT

    The extent is stored in 'fontExtent'.

    NOTES

    Neither effects of algorithmic additions nor rp_TxSpacing is included
    when the bounding box and font size are calculated. Note that te_Width
    only will be negative when FPF_REVPATH is specified for the font; left
    moving characters are ignored considering the font width (right moving
    character when FPF_REVPATH is set), but affects the bounding box size.

    EXAMPLE

    BUGS

    SEE ALSO

    TextExtent(), <graphics/text.h>

    INTERNALS

    HISTORY

    990617   SDuvan  Implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    WORD i;			   /* Loop variable */
    WORD maxwidth = -0x7fff;
    WORD minwidth =  0x7fff;
    WORD width    =  0;
    
    for(i = 0; i <= font->tf_HiChar - font->tf_LoChar; i++)
    {
	WORD kern;                 /* Kerning value for the character */
	WORD wspace;               /* Width of character including CharSpace */
	
	kern = 0;
	
	if(font->tf_CharKern != NULL)
	    kern = ((WORD *)font->tf_CharKern)[i];
	
	minwidth = min(minwidth, kern);
	
	/* tf_CharLoc[2*i+1] contains the width of the glyph bitmap.
	   But in AROS tf_CharLoc is being handled like an LONG array,
	   not a WORD array, so the width is tf_CarLoc[i] & 0xFFFF */
	maxwidth = max(maxwidth, kern + ((((LONG *)font->tf_CharLoc)[i]) & 0xFFFF));
	
	if(font->tf_CharSpace != NULL)
	    wspace = kern + ((WORD *)font->tf_CharSpace)[i];
	else
	    /* Is it possible to have kerning values but no CharSpace? */
	    wspace = kern + font->tf_XSize;


	if(font->tf_Flags & FPF_REVPATH)
	    width = min(wspace, width);
	else
	    width = max(wspace, width);
    }
 
    fontExtent->te_Width       = width;
    fontExtent->te_Height      = font->tf_YSize;
    fontExtent->te_Extent.MinX = minwidth;
    fontExtent->te_Extent.MaxX = maxwidth - 1;
    fontExtent->te_Extent.MinY = -font->tf_Baseline;
    fontExtent->te_Extent.MaxY = font->tf_YSize - font->tf_Baseline - 1;
    
    AROS_LIBFUNC_EXIT
} /* FontExtent */
