/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
	struct GfxBase *, GfxBase, 179, Graphics)

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

    D(bug("DeInitRP()\n"));

#if NEW_DRIVERDATA_CODE
    KILL_DRIVERDATA(rp, GfxBase);
#else
    if ( rp->Flags & RPF_DRIVER_INITED )
    {
    	D(bug("RP inited, rp=%p, %flags=%d=\n", rp, rp->Flags));
		 
        if (GetDriverData(rp)->dd_RastPort == rp) 
	{
	    D(bug("Calling DeInitDriverData\n"));
	    obsolete_DeinitDriverData (rp, GfxBase);
	}
    }
#endif
 
    AROS_LIBFUNC_EXIT
    
} /* DeinitRastPort */
