/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for initializing graphics.hidd
    Lang: english
*/

#include "graphics_intern.h"

#include <aros/debug.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>

void driver_expunge (struct GfxBase * GfxBase)
{
    /* Try to free some other stuff */
    if (SDD(GfxBase)->framebuffer) {
	OOP_DisposeObject(SDD(GfxBase)->framebuffer);
	SDD(GfxBase)->framebuffer = NULL;
    }

    if ( SDD(GfxBase)->planarbm_cache ) {
	delete_object_cache( SDD(GfxBase)->planarbm_cache, GfxBase );
	SDD(GfxBase)->planarbm_cache = NULL;
    }

    if ( SDD(GfxBase)->gc_cache ) {
	delete_object_cache( SDD(GfxBase)->gc_cache, GfxBase );
	SDD(GfxBase)->gc_cache = NULL;
    }

    if ( SDD(GfxBase)->fakegfx_inited ) {
        OOP_DisposeObject(SDD(GfxBase)->gfxhidd);
	SDD(GfxBase)->fakegfx_inited = FALSE;
    }

    if ( SDD(GfxBase)->gfxhidd_orig ) {
	OOP_DisposeObject( SDD(GfxBase)->gfxhidd_orig );
	SDD(GfxBase)->gfxhidd_orig = NULL;
    }
     
    return;
}

/*****i***********************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH1(BOOL , LateGfxInit,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, gfxhiddname, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 181, Graphics)

/*  FUNCTION
	This function permits late initialization
	of Graphics (After DOS but *before* Graphics is used, i.e.
	before the first view/screen has been set up).

    INPUTS

    RESULT
    	success - If TRUE initialization went OK.

    NOTES
	This function is private and AROS specific. At the moment it is being constantly
	modified, so don't expect anything from it at all.

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

    struct MonitorSpec *mspc;

    EnterFunc(bug("driver_LateGfxInit(gfxhiddname=%s)\n", gfxhiddname));

    /* First we prepare a MonitorSpec structure and insert it into the list.
       In future display drivers will need to do this themselves, so LateGfxInit() function
       will not be needed */

    /* Check if the monitor is already installed */
    mspc = OpenMonitor(gfxhiddname, 0);
    if (mspc) {
        D(bug("[driver_LateGfxInit] Driver is already present\n"));
        CloseMonitor(mspc);
	return TRUE;
    }

    /* Set up a MonitorSpec structure. */
    mspc = GfxNew(MONITOR_SPEC_TYPE);
    if (mspc) {
        mspc->ms_Special = GfxNew(SPECIAL_MONITOR_TYPE);
        if (mspc->ms_Special) {
	    ULONG l = strlen(gfxhiddname) + 1;

	    mspc->ms_Node.xln_Name = AllocMem(l, MEMF_ANY);
	    if (mspc->ms_Node.xln_Name) {
		CopyMem(gfxhiddname, mspc->ms_Node.xln_Name, l);
		D(bug("[GFX] Adding monitor driver: %s\n", mspc->ms_Node.xln_Name));
	    } else
	        GfxFree(&mspc->ms_Special->spm_Node);
	}
    }

    if (!mspc || !mspc->ms_Node.xln_Name) {
        if (mspc)
	    GfxFree(&mspc->ms_Node);
	return FALSE;
    }

    NEWLIST(&mspc->DisplayInfoDataBase);
    InitSemaphore(&mspc->DisplayInfoDataBaseSemaphore);

    ObtainSemaphoreShared(GfxBase->MonitorListSemaphore);
    AddTail(&GfxBase->MonitorList, (struct Node *)mspc);
    ReleaseSemaphore(GfxBase->MonitorListSemaphore);

    /* Next we are going to switch over to the new driver. This part is a 100% hack */

    /* This OpenMonitor() will take care about driver setup */
    if (!OpenMonitor(gfxhiddname, 0))
        return FALSE;

    /* If everything is ok, unload the old driver.
       Note that driverdata pointer of its MonitorSpec will not be
       cleared (and the data itself will not be deallocated), so the
       driver will never be loaded again, and its object will always
       be NULL. */
    if (GfxBase->default_monitor)
        driver_expunge(GfxBase);
    D(bug("[GFX] Old driver removed\n"));

    /* It's time to activate the new driver */
    GfxBase->current_monitor = mspc;
    GfxBase->default_monitor = mspc;
    GfxBase->natural_monitor = mspc;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
