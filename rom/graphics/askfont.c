/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/text.h>
#include <proto/graphics.h>

	AROS_LH2(void, AskFont,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(struct TextAttr *, textAttr, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 79, Graphics)

/*  FUNCTION
	Query the attributes of the current font in a RastPort.

    INPUTS
	rp - Query this RastPort.

    RESULT
	textAttr will contain a description of the font.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    textAttr->ta_Name  = rp->Font->tf_Message.mn_Node.ln_Name;
    textAttr->ta_YSize = rp->Font->tf_YSize;
    textAttr->ta_Style = rp->Font->tf_Style;
    textAttr->ta_Flags = rp->Font->tf_Flags;

    AROS_LIBFUNC_EXIT
} /* AskFont */
