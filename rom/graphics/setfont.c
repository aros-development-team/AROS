/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log

    Desc: Graphics function SetFont()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <graphics/text.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <graphics/text.h>
	#include <clib/graphics_protos.h>

	AROS_LH2(void, SetFont,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(struct TextFont *, textFont, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 11, Graphics)

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

    driver_SetFont (rp, textFont, GfxBase);

    AROS_LIBFUNC_EXIT
} /* SetFont */
