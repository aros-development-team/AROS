/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
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

	AROS_LH1(BOOL , LateGfxInit,

/*  SYNOPSIS */
	AROS_LHA(APTR, data, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 181, Graphics)

/*  FUNCTION
	This function permits late initalization
	of gfx (After dos but *before* graphics is used, ie.
	before the first view/screen has been set up).
	Can be used to load gfx hidds from disk and initializing it.
	The function only passes the data through to the
	driver.

    INPUTS
	data - Data passed on to graphics driver.

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
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    GfxBase->gb_LayersBase = (ULONG *)OpenLibrary("layers.library", 0);

        /* We don't have a clue what 'data' parameter contains, but
           the graphics driver does.
        */

    return GfxBase->gb_LayersBase && driver_LateGfxInit(data, GfxBase);

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
