/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

LONG driver_Text (struct RastPort *, STRPTR, ULONG);

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	__AROS_LH3(LONG, Text,

/*  SYNOPSIS */
	__AROS_LA(struct RastPort *, rp, A1),
	__AROS_LA(STRPTR           , string, A0),
	__AROS_LA(unsigned long    , count, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 10, Graphics)

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

    return driver_Text (rp, string, count);

    __AROS_FUNC_EXIT
} /* Text */
