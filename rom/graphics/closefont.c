/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$	$Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/text.h>

void driver_CloseFont (struct TextFont *, struct GfxBase *);

/*****************************************************************************

    NAME */
	#include <graphics/text.h>
	#include <clib/graphics_protos.h>

	__AROS_LH1(void, CloseFont,

/*  SYNOPSIS */
	__AROS_LHA(struct TextFont *, textFont, A1),

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct GfxBase *,GfxBase)

    driver_CloseFont (textFont, GfxBase);

    __AROS_FUNC_EXIT
} /* CloseFont */
