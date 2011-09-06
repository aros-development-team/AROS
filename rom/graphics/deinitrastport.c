/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: AROS Graphics function DeinitRastPort()
    Lang: english
*/

#include <aros/debug.h>
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include "gfxfuncsupport.h"

void obsolete_DeinitDriverData(struct RastPort *, struct GfxBase *);

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, DeinitRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 141, Graphics)

/*  FUNCTION
	Frees the contents of a RastPort structure. The structure itself
	is not freed.

    INPUTS
	rp - The RastPort which contents are to be freed.

    RESULT
	None.

    NOTES
	You can initialize the RastPort again via InitRastPort() but
	you must not use any other graphics function with it before
	that.

    EXAMPLE

    BUGS

    SEE ALSO
	InitRastPort()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("DeInitRastPort()\n"));

    KillDriverData(rp, GfxBase);
 
    AROS_LIBFUNC_EXIT
    
} /* DeinitRastPort */
