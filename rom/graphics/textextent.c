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
	AROS_LHA(STRPTR             , string, A0),
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

    textExtent->te_Width = TextLength(rp, string, count);
    textExtent->te_Height = rp->Font->tf_YSize;
    textExtent->te_Extent.MinX = 0;
    textExtent->te_Extent.MinY = -rp->Font->tf_Baseline;
    textExtent->te_Extent.MaxX = textExtent->te_Width - 1;
    textExtent->te_Extent.MaxY = textExtent->te_Height - 1 - rp->Font->tf_Baseline;

    AROS_LIBFUNC_EXIT
    
} /* TextExtent */
