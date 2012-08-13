/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CoerceMode()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(ULONG, CoerceMode,

/*  SYNOPSIS */
        AROS_LHA(struct ViewPort *, RealViewPort, A0),
        AROS_LHA(ULONG, MonitorID, D0),
        AROS_LHA(ULONG, Flags, D1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 156, Graphics)

/*  FUNCTION
	Determine the best mode in the MonitorID to coerce RealViewPort to.

    INPUTS
        RealViewPort - ViewPort to coerce
        MonitorID    - Monitor number to coerce to
                       (i.e. a mode masked with MONITOR_ID_MASK)
        Flags        - PRESERVE_COLORS - keep the number of bitplanes
                                         in the ViewPort
                       AVOID_FLICKER   - do not coerce to an interlace mode

    RESULT
        ID - ID of best mode to coerce to, or INVALID_ID if could not coerce

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* TODO: Write graphics/CoerceMode() */
    aros_print_not_implemented ("CoerceMode");

    return INVALID_ID;

    AROS_LIBFUNC_EXIT
} /* CoerceMode */
