/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function CloseMonitor()
    Lang: english
*/
#include <aros/debug.h>
#include <graphics/monitor.h>

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH1(LONG, CloseMonitor,

/*  SYNOPSIS */
        AROS_LHA(struct MonitorSpec *, monitor_spec, A0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 120, Graphics)

/*  FUNCTION

    INPUTS
        monitor_spec - pointer to a MonitorSpec opened via OpenMonitor(),
                       or NULL

    RESULT
        error - FALSE if MonitorSpec closed uneventfully
                TRUE if MonitorSpec could not be closed

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        OpenMonitor(), graphics/monitor.h

    INTERNALS

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT

#warning TODO: Write graphics/CloseMonitor()
    aros_print_not_implemented ("CloseMonitor");

    return FALSE;

    AROS_LIBFUNC_EXIT
} /* CloseMonitor */
