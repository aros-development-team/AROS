/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function Text()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH3(void, Text,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(STRPTR           , string, A0),
	AROS_LHA(ULONG            , count, D0),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    if (count) driver_Text (rp, string, count, GfxBase);

    AROS_LIBFUNC_EXIT
} /* Text */
