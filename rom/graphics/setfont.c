/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <graphics/text.h>

LONG driver_SetFont (struct RastPort *, struct TextFont *);

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <graphics/text.h>
	#include <clib/graphics_protos.h>

	__AROS_LH2(LONG, SetFont,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A1),
	__AROS_LHA(struct TextFont *, textFont, A0),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct GfxBase *,GfxBase)

    return driver_SetFont (rp, textFont);

    __AROS_FUNC_EXIT
} /* SetFont */
