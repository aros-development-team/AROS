/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log
    Desc:
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>

extern void driver_SetOutlinePen (struct RastPort *, ULONG, struct GfxBase *);

/*****************************************************************************

    NAME */
	#include <graphics/rastport.h>
	#include <clib/graphics_protos.h>

	__AROS_LH2(ULONG, SetOutlinePen,

/*  SYNOPSIS */
	__AROS_LHA(struct RastPort *, rp, A0),
	__AROS_LHA(ULONG,             pen, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 163, Graphics)

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
    ULONG oldPen;

    oldPen = rp->AOlPen;

    driver_SetOutlinePen (rp, pen, GfxBase);

    rp->AOlPen = pen;
    rp->Flags |= AREAOUTLINE;

    return oldPen;
    __AROS_FUNC_EXIT
} /* SetOutlinePen */
