/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MrgCop()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(ULONG, MrgCop,

/*  SYNOPSIS */
        AROS_LHA(struct View *, view, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 35, Graphics)

/*  FUNCTION
        Merge together the display, color, sprite and user coprocessor
		instructions into a single coprocessor instruction stream.
		
    INPUTS
        view - a pointer to the view structure whos coprocessor instructions
		       are to be merged.

    RESULT
        error - ULONG error value indicating either lack of memory to build the system copper lists,
		        or that MrgCop() has no work to do - ie there where no viewPorts in the list.

    NOTES
        Pre-v39 AmigaOS returns void.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
	AROS currently doesn't run on Amiga hardware, so we don't work with real copperlists. However
	we try to behave as if we work with them. So if the view is set as active, we immediately apply
	all changes.

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* If the given view is a current one, apply changes immediately */
    if (GfxBase->ActiView == view) {
        if (!DisplayView(view, GfxBase))
	    return MCOP_NO_MEM;
    }

    return MCOP_OK;

    AROS_LIBFUNC_EXIT
} /* MrgCop */
