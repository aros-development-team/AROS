/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function OpenMonitor()
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/monitor.h>

#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH2(struct MonitorSpec *, OpenMonitor,

/*  SYNOPSIS */
        AROS_LHA(STRPTR, monitor_name, A1),
        AROS_LHA(ULONG, display_id, D0),

/*  LOCATION */
        struct GfxBase *, GfxBase, 119, Graphics)

/*  FUNCTION

    INPUTS
        monitor_name - pointer to a null terminated string
        display_id   - optional 32 bit monitor/mode identifier

    RESULT
        mspc - pointer to an open MonitorSpec structure
               NULL if MonitorSpec could not be opened

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        CloseMonitor(), graphics/monitor.h

    INTERNALS
        Currently display_id parameter is ignored because all display modes
	are served by the same driver (which is the defailt one).
	In future this will change.

    HISTORY


******************************************************************************/
{
    AROS_LIBFUNC_INIT
    
    struct MonitorSpec *mspc;

    D(bug("[GFX] OpenMonitor(%s)\n", monitor_name));

    if (monitor_name) {

        ObtainSemaphoreShared(GfxBase->MonitorListSemaphore);

        for (mspc = (struct MonitorSpec *)GfxBase->MonitorList.lh_Head; mspc->ms_Node.xln_Succ; mspc = (struct MonitorSpec *)mspc->ms_Node.xln_Succ) {
	    if (!strcmp(monitor_name, mspc->ms_Node.xln_Name)) {
	        D(bug("[OpenMonitor] Found spec 0x%p\n", mspc));
	        break;
	    }
	}

	ReleaseSemaphore(GfxBase->MonitorListSemaphore);

	if (!mspc->ms_Node.xln_Succ) {
	    D(bug("[OpenMonitor] Monitor not found\n"));
	    return NULL;
	}
	
    }
    /* TODO: implement lookup by display_id */
    else
        mspc = GfxBase->default_monitor;

    /* Note that we do initialization inside a semaphore. This protects us from
       potential race condition when several tasks attempt to open the same monitor
       The semaphore we use here is not exactly for this purpose, but this will not harm */
    ObtainSemaphore(&mspc->DisplayInfoDataBaseSemaphore);

    if (driver_OpenMonitor(mspc, GfxBase))
        mspc->ms_OpenCount++;
    else
        mspc = NULL;

    ReleaseSemaphore(&mspc->DisplayInfoDataBaseSemaphore);

    return mspc;

    AROS_LIBFUNC_EXIT
} /* OpenMonitor */
