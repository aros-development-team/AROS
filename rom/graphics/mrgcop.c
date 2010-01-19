/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function MrgCop()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

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

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* We don't have copper and don't use copperlists, so just do nothing */
    return MCOP_OK;

    AROS_LIBFUNC_EXIT
} /* MrgCop */
