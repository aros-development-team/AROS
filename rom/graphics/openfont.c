/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log

    Desc: Graphics function OpenFont()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/text.h>

/*****************************************************************************

    NAME */
	#include <graphics/text.h>
	#include <clib/graphics_protos.h>

	AROS_LH1(struct TextFont *, OpenFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextAttr *, textAttr, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 12, Graphics)

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
    struct TextFont * font;

    font = driver_OpenFont (textAttr, GfxBase);

    if (font)
    {
	font->tf_Accessors ++;
    }

    return font;
    AROS_LIBFUNC_EXIT
} /* OpenFont */
