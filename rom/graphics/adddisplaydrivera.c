/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS-specific function for adding a display driver
    Lang: english
*/

#include <aros/debug.h>
#include <graphics/driver.h>
#include <hidd/compositing.h>
#include <oop/oop.h>

#include <proto/utility.h>

#include "graphics_intern.h"
#include "compositing_driver.h"
#include "dispinfo.h"

#define CL(x) ((OOP_Class *)x)
#define IS_CLASS(x, n) (CL(x)->ClassNode.ln_Name && (!strcmp(CL(x)->ClassNode.ln_Name, n)))

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(ULONG, AddDisplayDriverA,

/*  SYNOPSIS */
	AROS_LHA(APTR, gfxclass, A0),
	AROS_LHA(struct TagItem *, attrs, A1),
	AROS_LHA(const struct TagItem *, tags, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 107, Graphics)

/*  FUNCTION
	Add a display driver to the system.

    INPUTS
	gfxhidd - A pointer to an OOP class of the display driver
        attrs   - Additional attributes to supply to the driver class during
                  object creation
        tags    - An optional TagList describing how graphics.library should
            handle the driver. Valid tags are:

            DDRV_BootMode     - A boolean value telling that a boot mode
                                driver is being added. Boot mode drivers
                                will automatically shut down on next
                                AddDisplayDriverA() call, unless
			        DDRV_KeepBootMode = TRUE is specified.
                                Defaults to FALSE.
            DDRV_MonitorID    - Starting monitor ID to assign to the driver.
                                Use it with care. An attempt to add already
                                existing ID will fail with a DD_ID_EXISTS
                                code. By default the next available ID will
                                be picked up automatically.
            DDRV_ReserveIDs   - The number of subsequent monitor IDs to
                                reserve. Reserved IDs can be reused only
                                with DDRV_MonitorID tag. This tag is
                                provided as an aid to support possible
                                removable display devices. Defaults to 1.
            DDRV_KeepBootMode - Do not shut down boot mode drivers. Use this
                                tag if you are 100% sure that your driver
                                won't conflict with boot mode driver (like
                                VGA or VESA) and won't attempt to take over
                                its hardware. Defaults to FALSE.
            DDRV_ResultID     - A pointer to a ULONG location where the ID
                                assigned to your driver will be placed.
                                Useful if you reserve some ID for future use.
	    			Note that the returned ID will be the one
                                just assigned to your driver instance.
                                Increment it yourself in order to obtain
                                other reserved IDs.
            DDRV_IDMask       - A mask for separating the monitor ID from the
                                HIDD-specific part. This mask specifies what
                                mode ID bits are the monitor ID and what bits
                                actually specify the mode. The default value
                                is 0xFFFF0000.
	    			
                                Using the mask you can split your monitor ID
                                into 'sub-Ids'. Example:

                                Supplied tags: DDRV_IDMask, 0xFFFFFF00,
                                               DDRV_ResultID, &myid

	    			After a successful call, myid will contain the
                                base ID assigned by graphics.library to your
                                driver, let's say 0x00140000. However, since
                                you specified a longer mask, you leave only
                                one byte for mode designation, and reserve
                                the whole range of IDs from 0x001400xx to
                                0x0014FFxx for different instances of your
                                driver. They can now be used by specifying
                                DDRV_MonitorID with corresponding value.

                                Note that for this feature to work correctly,
                                you also need to override ID processing in
                                your driver class. Default methods provided
                                by the hidd.graphics.graphics base class
                                suppose that the whole lower word of the mode
                                ID specifies the display mode.

                                It is generally not allowed to specify
                                shorter masks than 0xFFFF0000. The only
                                driver which can do this is the Amiga(TM)
                                chipset driver, which needs to occupy the
                                reserved range of IDs from 0x0000xxxx to
                                0x000Axxxx. In any other case, supplying a
                                short mask will cause undefined behavior.

                                Since DDRV_ReserveIDs provides a simpler way
                                to reserve IDs for your driver (without the
                                need to override mode ID processing), this
                                option can be considered experimental and
                                even private. In fact the primary reason for
				it to exist is to provide support for
                                Amiga(tm) chipset driver.

    RESULT
    	error - One of following codes:

            DD_OK           - Operation completed OK.
            DD_NO_MEM       - There is not enough memory to set up internal
                              data.
            DD_ID_EXISTS    - Attempt to assign monitor IDs that are already
                              used.
            DD_IN_USE       - One of boot-mode drivers is in use and cannot
                              be shut down.
            DD_DRIVER_ERROR - Failure to create driver object.

    NOTES
	This function is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem *tag, *tstate = (struct TagItem *)tags;
    struct monitor_driverdata *mdd;
    ULONG FirstID = INVALID_ID;
    ULONG NextID;
    ULONG NumIDs = 1;
    ULONG IDMask = AROS_MONITOR_ID_MASK;
    BOOL keep_boot = FALSE;
    UWORD flags = 0;
    ULONG *ResultID = NULL;
    ULONG ret = DD_OK;

    EnterFunc(bug("AddDisplayDriverA(0x%p) <%s>\n", gfxclass, CL(gfxclass)->ClassNode.ln_Name));

    /*
     * MAGIC: Detect composition HIDD here.
     * This allows to hotplug it, and even (potentially) replace.
     */
    if (IS_CLASS(gfxclass, CLID_Hidd_Compositing))
    {
	ObtainSemaphore(&CDD(GfxBase)->displaydb_sem);
    	ret = composer_Install(gfxclass, GfxBase);
    	ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    	return ret;
    }

    /* First parse parameters */
    while ((tag = NextTagItem(&tstate)))
    {
    	switch (tag->ti_Tag)
    	{
    	case DDRV_MonitorID:
    	    FirstID = tag->ti_Data;
    	    break;

    	case DDRV_ReserveIDs:
    	    NumIDs = tag->ti_Data;
    	    break;

	case DDRV_IDMask:
	    IDMask = tag->ti_Data;
	    break;

	case DDRV_KeepBootMode:
	    keep_boot = tag->ti_Data;
	    break;

	case DDRV_BootMode:
	    flags = tag->ti_Data ? DF_BootMode : 0;
	    break;

	case DDRV_ResultID:
	    ResultID = (ULONG *)tag->ti_Data;
	    break;
	}
    }

    /* We lock for the entire function because we want to be sure that
       IDs will remain free during driver_Setup() */
    ObtainSemaphore(&CDD(GfxBase)->displaydb_sem);

    /* Default value for monitor ID */
    if (FirstID == INVALID_ID)
    {
	/*
	 * The logic here prevents ID clash if specified mask is wider than previous one.
	 * Example situation:
	 * 1. Add driver with mask = 0xFFFF0000. FirstID = 0x00100000, NextID = 0x00110000.
	 * 2. Add driver with mask = 0xF0000000. FirstID = 0x00110000, AND with this mask
	 *    would give monitor ID = 0.
	 * In order to prevent this, we make one more increment, so that in (2) FirstID becomes
	 * 0x10000000. The increment mechanism itself is explained below.
	 * Note that the adjustments happens only for automatic ID assignment. In case of manual
	 * one (DDRV_MonitorID specified) we suggest our caller knows what he does.
	 */
	FirstID = CDD(GfxBase)->last_id & IDMask;

	if (FirstID < CDD(GfxBase)->last_id)
	    FirstID += (~(IDMask & AROS_MONITOR_ID_MASK) + 1);
    }

    /*
     * Calculate next free ID.
     * Mechanism of increment calculation: we invert the mask and add 1 to it.
     * This way for example 0xFFFF0000 becomes 0x00010000.
     * Before doing this we make sure that mask used in this equation is not
     * longer than 0xFFFF0000, for proper ID counting.
     */
    NextID = FirstID + NumIDs * (~(IDMask & AROS_MONITOR_ID_MASK) + 1);
    D(bug("[AddDisplayDriverA] First ID 0x%08X, next ID 0x%08X\n", FirstID, NextID));

    /* First check if the operation can actually be performed */
    for (mdd = CDD(GfxBase)->monitors; mdd; mdd = mdd->next)
    {
    	/* Check if requested IDs are already allocated */
	if ((mdd->id >= FirstID && mdd->id < NextID))
	{
	    ret = DD_ID_EXISTS;
	    break;
	}

	/*
	 * Now check if boot mode drivers can really be unloaded.
	 * Display drivers can start playing with their hardware during
	 * object creation, so we need to check it before instantiating
	 * the given class.
	 */
	if (!keep_boot)
	{
	    /* The driver can be unloaded if it has nothing on display */
	    if ((mdd->flags & DF_BootMode) && (mdd->display))
	    {
	    	ret = DD_IN_USE;
	    	break;
	    }
	}
    }

    /*
     * Now, if everything is okay, we are ready to instantiate the driver.
     * A well-behaved driver must touch the hardware only in object, not
     * in class. This makes this function much safer. If we can't exit boot mode,
     * the driver will not be instantiated and hardware state will not be clobbered.
     */
    if (ret == DD_OK)
    {
    	OOP_Object *gfxhidd = OOP_NewObject(gfxclass, NULL, attrs);

	if (gfxhidd)
	{
	    D(bug("[AddDisplayDriverA] Installing driver\n"));

	    /* Attach system structures to the driver */
	    mdd = driver_Setup(gfxhidd, GfxBase);
	    D(bug("[AddDisplayDriverA] monitor_driverdata 0x%p\n", mdd));

	    if (mdd)
	    {
	    	struct monitor_driverdata *last, *old;

	    	mdd->id    =  FirstID;
	    	mdd->mask  =  IDMask;
	    	mdd->flags |= flags;

		if (CDD(GfxBase)->DriverNotify)
		{
		    /* Use mdd->gfxhidd here because it can be substituted by fakegfx object */
		    mdd->userdata = CDD(GfxBase)->DriverNotify(mdd, TRUE, CDD(GfxBase)->notify_data);
		}

	    	/* Remove boot mode drivers */
	    	if (!keep_boot)
	    	{
		    D(bug("[AddDisplayDriverA] Shutting down boot mode drivers\n"));
		    for (last = (struct monitor_driverdata *)CDD(GfxBase);; last = last->next)
		    {
		    	D(bug("[AddDisplayDriverA] Current 0x%p, next 0x%p\n", last, last->next));

		    	while (last->next && (last->next->flags & DF_BootMode))
		    	{
		            old = last->next;
			    D(bug("[AddDisplayDriverA] Shutting down driver 0x%p (ID 0x%08lX, next 0x%p)\n", old, old->id, old->next));

			    last->next = old->next;
			    driver_Expunge(old, GfxBase);
			    D(bug("[AddDisplayDriverA] Shutdown OK, next 0x%p\n", last->next));
			}

		    	/*
		    	 * We check this condition here explicitly because last->next is modified inside loop body.
		         * If we check it in for() statement, last = last->next will be executed BEFORE the check,
		         * and NULL pointer may be hit.
		         */
		    	if (!last->next)
			    break;
		    }
	    	}

	    	/* Insert the driverdata into chain, sorted by ID */
	    	D(bug("[AddDisplayDriverA] Inserting driver 0x%p, ID 0x%08lX\n", mdd, mdd->id));
	    	for (last = (struct monitor_driverdata *)CDD(GfxBase); last->next; last = last->next)
	    	{
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
	    	if (ResultID)
	    	    *ResultID = FirstID;
	    }
    	    else /* if (mdd) */
    	    {
    	    	OOP_DisposeObject(gfxhidd);
		ret = DD_NO_MEM;
	    }
	}
	else /* if (gfxhidd) */
	{
	    ret = DD_DRIVER_ERROR;
	}
    } /* if (ret == DD_OK) */

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    /* Set the first non-boot non-planar driver as default */
    if ((ret == DD_OK) && (!GfxBase->default_monitor) && (!(mdd->flags & DF_BootMode)))
    {
	/*
	 * Amiga(tm) chipset driver does not become a default.
	 * This is done because RTG modes (if any) are commonly preferred
	 * over it.
	 * TODO: in future some prefs program could be implemented. It would
	 * allow the user to describe the physical placement of several displays
	 * in his environment, and explicitly set the preferred display.
	 */
	if (!IS_CLASS(gfxclass, "hidd.gfx.amigavideo"))
	{
	    /*
	     * graphics.library uses struct MonitorSpec pointers for historical reasons,
	     * so we satisfy it.
	     * Here we just get the first available sync object from the driver and
	     * set default_monitor fo its MonitorSpec. This allows BestModeIDA() to
	     * obtain preferred monitor back from this MonitorSpec (by asking the associated
	     * sync object about its parent driver).
	     *
	     * TODO:
	     * Originally display drivers in AmigaOS had a concept of "preferred mode ID".
	     * Every driver supplied own hardcoded ID which can be retrieved by GetDisplayInfoData()
	     * in MonitorInfo->PreferredModeID. Currently AROS does not implement this concept.
	     * However this sync could be a preferred mode's sync.
	     * It needs to be researched what exactly this mode ID is. Implementing this concept would
	     * improve AmigaOS(tm) compatibility.
	     */
    	    OOP_Object *sync = HIDD_Gfx_GetSync(mdd->gfxhidd_orig, 0);

	    OOP_GetAttr(sync, aHidd_Sync_MonitorSpec, (IPTR *)&GfxBase->default_monitor);
	}
    }

    D(bug("[AddDisplayDriverA] Returning %u\n", ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
