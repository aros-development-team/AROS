/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function Move()
    Lang: english
*/
#include <graphics/rastport.h>
#include "graphics_intern.h"
#include "gfxfuncsupport.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH3(void, Move,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(WORD             , x, D0),
	AROS_LHA(WORD             , y, D1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 40, Graphics)

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

    FIX_GFXCOORD(x);
    FIX_GFXCOORD(y);
    
    rp->cp_x = x;
    rp->cp_y = y;
    rp->linpatcnt = 15;
    
    AROS_LIBFUNC_EXIT
    
} /* Move */
