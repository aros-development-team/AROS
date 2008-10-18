/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function VideoControl()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/view.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(ULONG, VideoControl,

/*  SYNOPSIS */
        AROS_LHA(struct ColorMap *, cm, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct GfxBase *, GfxBase, 118, Graphics)

/*  FUNCTION

    INPUTS
        cm   - pointer to struct ColorMap obtained via GetColorMap()
        tags - pointer to a table of videocontrol tagitems

    RESULT
        error - 0 if no error ocurred in the control operation
                non-0 if bad colormap pointer, no tagitems or bad tag
    NOTES
	Not implemented

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning TODO: Write graphics/VideoControl()
    aros_print_not_implemented ("VideoControl");

    return 1;

    AROS_LIBFUNC_EXIT
} /* VideoControl */
