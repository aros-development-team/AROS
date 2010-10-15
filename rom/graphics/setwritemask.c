/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$    $Log

    Desc: Graphics function SetWriteMask()
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH2(ULONG, SetWriteMask,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp,   A0),
	AROS_LHA(ULONG            , mask, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 164, Graphics)

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

#if 0
    rp->Mask = mask;
    return TRUE;
#else
#warning SetWriteMask not yet implemented
    return FALSE;
#endif
 
    AROS_LIBFUNC_EXIT
    
} /* SetWriteMask */
