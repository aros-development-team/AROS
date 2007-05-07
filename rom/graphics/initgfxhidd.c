/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for initializing graphics.hidd
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <oop/oop.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(BOOL , InitGfxHidd,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, hiddBase, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 181, Graphics)

/*  FUNCTION
	This function lets graphics.library initialize
	the supplied gfx.hidd

    INPUTS
	hiddBase - Library base of hidd to use for gfx.

    RESULT
    	success - If TRUE initialization went OK.

    NOTES
	This function is private and AROS specific.

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
    
    Class *cl;

    cl = AROS_LVO_CALL0(Class *, struct Library *, hiddBase, 5, );
    if (cl)
    {
    
    	/* Create a new GfxHidd object */
	
	
	
	return TRUE;
    }
    

    
    return FALSE;

    AROS_LIBFUNC_EXIT
} /* InitGfxHidd */
