/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: AROS-specific function for adding a display driver
*/

#include <aros/debug.h>
#include <graphics/driver.h>
#include <hidd/compositor.h>
#include <oop/oop.h>

#include <proto/utility.h>

#include <string.h>

#include "graphics_intern.h"
#include "graphics_driver.h"
#include "graphics_compositor.h"
#include "dispinfo.h"

#define CL(x) ((OOP_Class *)x)
#define IS_CLASS(x, n) (CL(x)->ClassNode.ln_Name && (!strcmp(CL(x)->ClassNode.ln_Name, n)))

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

        AROS_LH3(ULONG, AddDisplayDriverA,

/*  SYNOPSIS */
        AROS_LHA(APTR, gfxclass, A0),
        AROS_LHA(const struct TagItem *, attrs, A1),
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
    struct gfxdisplay_data *mdd;
    struct gfxdriver_data *cfg;
    ULONG ret = DD_OK;

    EnterFunc(bug("AddDisplayDriverA(0x%p) <%s>\n", gfxclass, CL(gfxclass)->ClassNode.ln_Name));

    /*
     * MAGIC: Detect composition HIDD here.
     * This allows to hotplug it, and even (potentially) replace.
     */
    if (IS_CLASS(gfxclass, CLID_Hidd_Compositor))
    {
	ObtainSemaphore(&CDD(GfxBase)->displaydb_sem);
    	ret = compositor_Install(gfxclass, GfxBase);
    	ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    	return ret;
    }

    cfg = AllocMem(sizeof(struct gfxdriver_data), MEMF_ANY);
    if (!cfg)
        return DD_NO_MEM;

    /* initialise the base config */
    cfg->drv_class = gfxclass;
    cfg->drv_idstore = NULL;
    cfg->drv_idbase = INVALID_ID;
    cfg->drv_idnext = 0;
    cfg->drv_idcnt = 1;
    cfg->drv_idmask = AROS_MONITOR_ID_MASK;
    cfg->drv_flags = 0;

    /* First parse parameters */
    while ((tag = NextTagItem(&tstate)))
    {
    	switch (tag->ti_Tag)
    	{
    	case DDRV_MonitorID:
    	    cfg->drv_idbase = tag->ti_Data;
    	    break;

    	case DDRV_ReserveIDs:
    	    cfg->drv_idcnt = tag->ti_Data;
    	    break;

	case DDRV_IDMask:
	    cfg->drv_idmask = tag->ti_Data;
	    break;

	case DDRV_BootMode:
            if (tag->ti_Data)
                cfg->drv_flags |= DF_BootMode;
            else
                cfg->drv_flags &= ~DF_BootMode;
	    break;

	case DDRV_ResultID:
	    cfg->drv_idstore = (ULONG *)tag->ti_Data;
	    break;
	}
    }

    /* We lock for the entire function because we want to be sure that
       IDs will remain free during driver_Setup() */
    ObtainSemaphore(&CDD(GfxBase)->displaydb_sem);

    /* Default value for monitor ID */
    if (cfg->drv_idbase == INVALID_ID)
    {
	/*
	 * The logic here prevents ID clash if specified mask is wider than previous one.
	 * Example situation:
	 * 1. Add driver with mask = 0xFFFF0000. BaseID = 0x00100000, NextID = 0x00110000.
	 * 2. Add driver with mask = 0xF0000000. BaseID = 0x00110000, AND with this mask
	 *    would give monitor ID = 0.
	 * In order to prevent this, we make one more increment, so that in (2) BaseID becomes
	 * 0x10000000. The increment mechanism itself is explained below.
	 * Note that the adjustments happens only for automatic ID assignment. In case of manual
	 * one (DDRV_MonitorID specified) we suggest our caller knows what he does.
	 */
	cfg->drv_idbase = GFXPRIVATE_MODELAST & cfg->drv_idmask;

	if (cfg->drv_idbase < GFXPRIVATE_MODELAST)
	    cfg->drv_idbase += (~(cfg->drv_idmask & AROS_MONITOR_ID_MASK) + 1);
    }

    /*
     * Calculate next free ID.
     * Mechanism of increment calculation: we invert the mask and add 1 to it.
     * This way for example 0xFFFF0000 becomes 0x00010000.
     * Before doing this we make sure that mask used in this equation is not
     * longer than 0xFFFF0000, for proper ID counting.
     */
    cfg->drv_idnext = cfg->drv_idbase + cfg->drv_idcnt * (~(cfg->drv_idmask & AROS_MONITOR_ID_MASK) + 1);
    D(bug("[graphics.library] %s: First ID 0x%08X, next ID 0x%08X\n", __func__, cfg->drv_idbase, cfg->drv_idnext));

    /* First check if the operation can actually be performed */
    for (mdd = (struct gfxdisplay_data *)CDD(GfxBase); mdd; mdd = mdd->display_next)
    {
    	/* Check if requested IDs are already allocated */
	if ((mdd->display_idbase >= cfg->drv_idbase && mdd->display_idbase < cfg->drv_idnext))
	{
	    ret = DD_ID_EXISTS;
	    break;
	}
    }

    /* Now, we are ready to add the driver for the system to find. */
    if ((ret == DD_OK) && !(driver_Setup(cfg, attrs, FALSE, GfxBase)))
        ret = DD_DRIVER_ERROR;

    ReleaseSemaphore(&CDD(GfxBase)->displaydb_sem);

    D(bug("[graphics.library] %s: Returning %u\n", __func__, ret));
    return ret;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
