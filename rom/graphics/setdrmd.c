/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log

    Desc: Graphics function SetDrMd()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	AROS_LH2(void, SetDrMd,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(unsigned long    , drawMode, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 59, Graphics)

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

    driver_SetDrMd (rp, drawMode, GfxBase);

    rp->DrawMode = drawMode;

    AROS_LIBFUNC_EXIT
} /* SetDrMd */
