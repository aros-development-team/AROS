/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$	$Log

    Desc: Graphics function InitRastPort()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <clib/exec_protos.h>
#include <string.h>

static const struct RastPort defaultRastPort =
{
    0,
};

/*****************************************************************************

    NAME */
	#include <clib/graphics_protos.h>

	AROS_LH1(void, InitRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 33, Graphics)

/*  FUNCTION
	OBSOLETE - DO NOT USE THIS FUNCTION.

	Initializes a RastPort structure.

    INPUTS
	rp - The RastPort to initialize.

    RESULT
	None.

    NOTES
	Due to RTG, this function should not be used. If you do, you might
	experience memory loss.

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

    CopyMem ((UBYTE *)&defaultRastPort, rp, sizeof (struct RastPort));

    driver_InitRastPort (rp, GfxBase);

    AROS_LIBFUNC_EXIT
} /* InitRastPort */
