/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function OpenMonitor()
    Lang: english
*/

#include <aros/atomic.h>
#include <aros/debug.h>
#include <graphics/monitor.h>
#include <proto/oop.h>

#include <stddef.h>

#include "graphics_intern.h"
#include "dispinfo.h"

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
    
    struct MonitorSpec *mspc = NULL;

    D(bug("[GFX] OpenMonitor(%s)\n", monitor_name));

    if (monitor_name) {
	if (stricmp(monitor_name, DEFAULT_MONITOR_NAME)) {
	    ObtainSemaphoreShared(GfxBase->MonitorListSemaphore);

	    /* TODO: use FindName() here, however this is possible only after switch to
	       ABI v1 (because otherwise struct Node and struct ExtendedNode have different layout */
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
	} else
	    mspc = GfxBase->default_monitor;
    } else if (display_id != INVALID_ID) {
        struct MonitorInfo info;
	
	if (GetDisplayInfoData(NULL, (UBYTE *)&info, sizeof(info), DTAG_MNTR, display_id) >= offsetof(struct MonitorInfo, ViewPosition))
	    mspc = info.Mspc;
    } else
        mspc = GfxBase->default_monitor;

    if (mspc)
        AROS_ATOMIC_INC(mspc->ms_OpenCount);

    return mspc;

    AROS_LIBFUNC_EXIT
} /* OpenMonitor */
