/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS-specific function for adding a display driver
    Lang: english
*/

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
	    DDRV_ResultID     - A pointer to ULONG location where ID assigned to your driver
	    			will be placed. Useful if you reserve some ID for future use.
	    			Note that returned ID will be the one just assigned to your
	    			driver instance. Increment it yourself in order to obtain
	    			other reserved IDs.
	    DDRV_IDMask	      - A mask for separating monitor ID from HIDD-specific part.
	    			This mask specifies what mode ID bits are monitor ID and
	    			what bits actually specify the mode. A default value is
	    			0xFFFF0000.
	    			
	    			Using the mask you can split your monitor ID into 'sub-Ids'.
	    			Example:

	    			Supplied tags: DDRV_IDMask, 0xFFFFFF00, DDRV_ResultID, &myid

	    			After succesfull call myid will contain base ID assigned by
	    			graphics.library to your driver, let's say 0x00140000. However,
	    			since you specified longer mask, you leave only one byte for mode
	    			designation, and reserve the whole range of IDs from 0x001400xx to
	    			0x0014FFxx for different instances of your driver. They can now be
	    			used by specifying DDRV_MonitorID with corresponding value.

				Note that for this feature to work correctly, you also need to override
				mode ID processing in your driver class. Default methods provided by
				hidd.graphics.graphics base class suppose that the whole lower word
				of mode ID specifies the display mode.

				It is generally not allowed to specify shorter masks than 0xFFFF0000.
				The only driver which can do this is Amiga(tm) chipset driver, which
				need to occupy the reserved range of IDs from 0x0000xxxx to 0x000Axxxx.
				In any other case supplying short mask will cause undefined behavior.

				Since DDRV_ReserveIDs provide simpler way to reserve IDs for your driver
				(without the need to override mode ID processing), this	option can be
				considered experimental and even private. In fact the primary reason for
				it to exist is to provide support for Amiga(tm) chipset	driver.

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
    ULONG NumIDs, IDMask;
    ULONG ret = DD_OK;

    EnterFunc(bug("AddDisplayDriverA(0x%p)\n", gfxhidd));

    /* We lock for the entire function because we want to be sure that
       IDs will remain free during driver_Setup() */
    ObtainSemaphore(&CDD(GfxBase)->displaydb_sem);

    FirstID = GetTagData(DDRV_MonitorID, CDD(GfxBase)->last_id, tags);
    NumIDs  = GetTagData(DDRV_ReserveIDs, 1, tags);
    IDMask  = GetTagData(DDRV_IDMask, AROS_MONITOR_ID_MASK, tags);

    /*
     * Calculate next free ID.
     * Mechanism of increment calculation: we invert the mask and add 1 to it.
     * This way for example 0xFFFF0000 becomes 0x00010000.
     * Before doing this we make sure that mask used in this equation is not
     * longer than 0xFFFF0000, for proper ID counting.
     */
    NextID = FirstID + NumIDs * (~(IDMask & AROS_MONITOR_ID_MASK) + 1);
    D(bug("[AddDisplayDriverA] First ID 0x%08X, next ID 0x%08X\n", FirstID, NextID));

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

	if (mdd)
	{
	    BOOL keep_boot;
	    struct monitor_driverdata *last, *old;
	    ULONG *ResultID;

	    mdd->id   = FirstID;
	    mdd->mask = IDMask;

	    if (GetTagData(DDRV_BootMode, FALSE, tags))
	        mdd->flags |= DF_BootMode;

	    if (CDD(GfxBase)->DriverNotify)
		/* Use mdd->gfxhidd here because it can be substituted by fakegfx object */
		mdd->userdata = CDD(GfxBase)->DriverNotify(mdd, TRUE, CDD(GfxBase)->notify_data);

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
		if (mdd->id < last->next->id)
		    break;
	    }
	    D(bug("[AddDisplayDriverA] Inserting after 0x%p\n", last));
	    mdd->next = last->next;
	    last->next = mdd;

	    /* Remember next available ID */
	    if (NextID > CDD(GfxBase)->last_id)
		CDD(GfxBase)->last_id = NextID;

	    /* Return the assigned ID if the caller asked to do so */
    	    ResultID = (ULONG *)GetTagData(DDRV_ResultID, 0, tags);
	    if (ResultID)
	    	*ResultID = FirstID;

	} else
	    ret = DD_NO_MEM;
    }

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    D(bug("[AddDisplayDriverA] Returning %u\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
