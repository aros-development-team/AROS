/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function SetAPen()
    Lang: english
*/
#include "graphics_intern.h"
#include <proto/oop.h>
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH2(void, SetAPen,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(ULONG            , pen, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 57, Graphics)

/*  FUNCTION
	Set primary pen for rastport

    INPUTS
	rp  - RastPort
	pen - pen number (0...255)

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

    struct gfx_driverdata *dd;

    if (OBTAIN_DRIVERDATA(rp, GfxBase))
    {
	dd = GetDriverData(rp);
	if (dd)
	{
            struct TagItem col_tags[]=
	    {
		{ aHidd_GC_Foreground, 0},
		{ TAG_DONE	    	}
	    };

	    col_tags[0].ti_Data = rp->BitMap ? BM_PIXEL(rp->BitMap, pen & PEN_MASK) : pen;
	    OOP_SetAttrs( dd->dd_GC, col_tags );

	}
	RELEASE_DRIVERDATA(rp, GfxBase);
    }

    rp->FgPen = pen;
    rp->linpatcnt = 15;

    AROS_LIBFUNC_EXIT
} /* SetAPen */
