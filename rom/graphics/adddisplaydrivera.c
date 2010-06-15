/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for initializing graphics.hidd
    Lang: english
*/

#define DEBUG 1

#include <aros/debug.h>
#include <graphics/driver.h>
#include <oop/oop.h>
#include <proto/utility.h>

#include "graphics_intern.h"
#include "dispinfo.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(ULONG, AddDisplayDriverA,

/*  SYNOPSIS */
	AROS_LHA(APTR, gfxhidd, A0),
	AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 181, Graphics)

/*  FUNCTION
	Add a display driver to the system.

    INPUTS
	gfxhidd - A newly created driver object
	tags    - An optional TagList. Valid tags are:

	    DDRV_BootMode     - A boolean value telling that a boot mode driver
			        is being added. Boot mode drivers will automatically
			        shutdown on next AddDisplayDriverA() call, unless
			        DDRV_KeepBootMode = TRUE is specified. Defaults to FALSE.
	    DDRV_MonitorID    - Starting monitor ID to assign to the driver. Use it
				with care. Attempt to add already existing ID will
				fail with DD_ID_EXISTS code. By default a next available
				ID will be picked up automatically.
	    DDRV_ReserveIDs   - A number of subsequent monitor IDs to reserve. Reserved IDs
				can be reused only with DDRV_MonitorID tag. This tag is
				provided as an aid to support possible removable display
				devices. Defaults to 1.
	    DDRV_KeepBootMode - Do not shut down boot mode drivers. Use this tag if you
				are 100% sure that your driver won't conflict with boot mode
				driver (like VGA or VESA) and won't attempt to take over its
				hardware. Defaults to FALSE.

    RESULT
    	error - One of following codes:

	    DD_OK        - Operation completed OK.
	    DD_NO_MEM	 - There is not enough memory to set up internal data.
	    DD_ID_EXISTS - Attempt to assign monitor IDs that are already used.

    NOTES
	This function is AROS-specific.

    EXAMPLE

    BUGS
	graphics.library tracks down usage of display drivers. If a driver currently
	has something on display, it will not be shut down, even if it's boot mode
	driver. This can cause problems if the new driver attempts to take over
	the same hardware (for example native mode driver vs VESA driver). So be careful
	while adding new display drivers on a working system. Know what you do.

	There's no way to know which IDs have been reserved when using DDRV_ReserveIDs.

	These issues will be addressed in future, this API is raw and incomplete.

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct monitor_driverdata *mdd;
    ULONG FirstID, NextID;
    ULONG NumIDs;
    ULONG ret = DD_OK;

    EnterFunc(bug("AddDisplayDriverA(0x%p)\n", gfxhidd));

    /* We lock for the entire function because we want to be sure that
       IDs will remain free during driver_Setup() */
    ObtainSemaphore(&CDD(GfxBase)->displaydb_sem);

    FirstID = GetTagData(DDRV_MonitorID, CDD(GfxBase)->last_id, tags);
    NumIDs = GetTagData(DDRV_ReserveIDs, 1, tags);
    NextID = FirstID + (NumIDs << AROS_MONITOR_ID_SHIFT);

    /* First check if requested IDs are already allocated */
    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next) {
	if ((mdd->id >= FirstID && mdd->id < NextID)) {
	    ret = DD_ID_EXISTS;
	    break;
	}
    }

    if (ret == DD_OK) {
	/* Attach system structures to the driver */
	D(bug("[AddDisplayDriverA] Installing driver\n"));
	mdd = driver_Setup(gfxhidd, GfxBase);
	D(bug("[AddDisplayDriverA] monitor_driverdata 0x%p\n", mdd));
	if (mdd) {
	    BOOL keep_boot;
	    struct monitor_driverdata *last, *old;

	    mdd->id   = FirstID;
	    mdd->mask = AROS_MONITOR_ID_MASK;
	    if (GetTagData(DDRV_BootMode, FALSE, tags))
	        mdd->flags |= DF_BootMode;

	    /* Remove boot mode drivers if needed */
	    keep_boot = GetTagData(DDRV_KeepBootMode, FALSE, tags);
	    if (!keep_boot) {
		D(bug("[AddDisplayDriverA] Shutting down boot mode drivers\n"));
		for (last = (struct monitor_driverdata *)CDD(GfxBase);; last = last->next) {
		    D(bug("[AddDisplayDriverA] Current 0x%p, next 0x%p\n", last, last->next));
		    /* Do not shut down the driver if it displays something.
		       Experimental and will cause problems in certain cases. */
		    while (last->next && (last->next->flags & DF_BootMode) && (!last->next->display)) {
		        old = last->next;
			D(bug("[AddDisplayDriverA] Shutting down driver 0x%p (ID 0x%08lX, next 0x%p)\n", old, old->id, old->next));
			last->next = old->next;
			driver_Expunge(old, GfxBase);
			D(bug("[AddDisplayDriverA] Shutdown OK, next 0x%p\n", last->next));
		    }

		    /* We check this condition here explicitly because last->next is modified inside loop body.
		       If we check it in for() statement, last = last->next will be executed BEFORE the check,
		       and NULL pointer may be hit. */
		    if (!last->next)
			break;
		}
	    }

	    /* Insert the driverdata into chain, sorted by ID */
	    D(bug("[AddDisplayDriverA] Inserting driver 0x%p, ID 0x%08lX\n", mdd, mdd->id));
	    for (last = (struct monitor_driverdata *)CDD(GfxBase); last->next; last = last->next) {
	        D(bug("[AddDisplayDriverA] Current 0x%p, next 0x%p, ID 0x%08lX\n", last, last->next, last->next->id));
		if (mdd->id > last->next->id)
		    break;
	    }
	    D(bug("[AddDisplayDriverA] Inserting after 0x%p\n", last));
	    mdd->next = last->next;
	    last->next = mdd;

	    /* Remember next available ID */
	    if (NextID > CDD(GfxBase)->last_id)
		CDD(GfxBase)->last_id = NextID;
	} else
	    ret = DD_NO_MEM;
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    D(bug("[AddDisplayDriverA] Returning %u\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
