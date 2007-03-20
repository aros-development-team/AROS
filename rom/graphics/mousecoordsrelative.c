/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for seeing if the underlying system provides relative or absolute mouse coords
    Lang: english
*/
#include "graphics_intern.h"
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH0(BOOL , MouseCoordsRelative,

/*  SYNOPSIS */

/*  LOCATION */
	struct GfxBase *, GfxBase, 183, Graphics)

/*  FUNCTION
	Tells whether mouse coordinates gotten from the below system are
	relative or absolute.

    INPUTS

    RESULT
    	relative - TRUE if relative, FALSE if absolute.

    NOTES
	This function is private and AROS specific.

    EXAMPLE

    BUGS
	There should not be need for such a function.
	All coordinates gotten should be relative.
	This is however difficult to do
	with HIDDs base on window systems and
	we use one window per screen.
    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    
    IPTR iswindowed;
    
    OOP_GetAttr(SDD(GfxBase)->gfxhidd, aHidd_Gfx_IsWindowed, &iswindowed);
    
    return iswindowed ? FALSE : TRUE;

    AROS_LIBFUNC_EXIT
    
} /* LateGfxInit */
