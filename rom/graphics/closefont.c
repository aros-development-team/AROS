/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$	$Log

    Desc: Graphics function CloseFont()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/text.h>

/*****************************************************************************

    NAME */
	#include <graphics/text.h>
	#include <clib/graphics_protos.h>

	AROS_LH1(void, CloseFont,

/*  SYNOPSIS */
	AROS_LHA(struct TextFont *, textFont, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 13, Graphics)

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

    textFont->tf_Accessors --;

    driver_CloseFont (textFont, GfxBase);

    AROS_LIBFUNC_EXIT
} /* CloseFont */
