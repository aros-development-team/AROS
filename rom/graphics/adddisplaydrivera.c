/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for initializing graphics.hidd
    Lang: english
*/

#include <aros/debug.h>
#include <oop/oop.h>

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
	tags    - An optional TagList.
		  Currently none of tags are defined, this is a WIP.

    RESULT
    	error - An error code or zero if everything went OK.
		Currently no error codes are defined, just check against zero
		or nonzero. Nonzero means error.

    NOTES
	This function is AROS-specific.

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

    struct monitor_driverdata *mdd;

    EnterFunc(bug("AddDisplayDriverA(0x%p)\n", gfxhidd));

    /* Attach system structures to the driver */
    mdd = driver_Setup(gfxhidd, GfxBase);
    D(bug("[AddDisplayDriverA] monitor_driverdata 0x%p\n", mdd));
    if (!mdd)
	return TRUE;

    mdd->mask = AROS_MONITOR_ID_MASK;

    /* The following is a temporary hack. We still have SDD(GfxBase)
       in some places and we still use only one driver */
    if (SDD(GfxBase)) {
        driver_Expunge(SDD(GfxBase), GfxBase);
        D(bug("[AddDisplayDriverA] Old driver removed\n"));
    }

    /* Add display modes from the new driver. Perhaps will go into to driver_Setup() in future */
    driver_Add(mdd, 1, GfxBase);

    D(bug("[AddDisplayDriverA] Installing new driver\n"));
    SDD(GfxBase) = mdd;

    /* Success */
    return 0;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
