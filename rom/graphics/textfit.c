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
	AROS_LHA(STRPTR             , string, A0),
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
    ULONG width;
    ULONG fit;

    if (constrainingExtent)
    {
	constrainingBitWidth  = constrainingExtent->te_Width;
	constrainingBitHeight = constrainingExtent->te_Height;
    }

    /* Font too high ? */
    if (rp->Font->tf_YSize > constrainingBitHeight)
	return 0;

    textExtent->te_Extent.MinX = 0;
    textExtent->te_Extent.MinY = 0;
    textExtent->te_Extent.MaxY = rp->Font->tf_YSize - 1;
    textExtent->te_Height = rp->Font->tf_YSize;

    textExtent->te_Extent.MaxX = 0;

    for (width = fit = 0; strLen; strLen--)
    {
	width += TextLength (rp, string, 1);

	if (width > constrainingBitWidth)
	    break;

	textExtent->te_Extent.MaxX = width - 1;
	string += strDirection;
	
	fit ++;
    }

    if (fit)
    {
	textExtent->te_Width = textExtent->te_Extent.MaxX
		             - textExtent->te_Extent.MinX + 1;
    }

    return fit;
    AROS_LIBFUNC_EXIT
} /* TextFit */
