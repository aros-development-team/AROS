/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private graphics function for initializing graphics.hidd
    Lang: english
*/

#include "graphics_intern.h"
#include "dispinfo.h"

#include <aros/debug.h>
#include <exec/memory.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>

/*****************************************************************************

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
	I think in future this function will accept pre-created driver object.
	Its job then will be to attach necessary system structures and append
	the driver to internal display mode database.

	Driver classes will be completely private, there's no need to give them
	names.

	In order to be able to use old drivers, a loader program will be provided
	for them.

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    OOP_Class *gfxclass;
    OOP_Object *gfxhidd;
    struct monitor_driverdata *mdd;

    EnterFunc(bug("LateGfxInit(gfxhiddname=%s)\n", gfxhiddname));

    /* Check if we are already using the given driver */
    mdd = SDD(GfxBase);
    if (mdd) {
        gfxclass = OOP_OCLASS(mdd->gfxhidd);
	D(bug("[LateGfxInit] Have current driver class 0x%p (%s)\n", gfxclass, gfxclass->ClassNode.ln_Name));
	if (!strcmp(gfxclass->ClassNode.ln_Name, gfxhiddname))
	    return TRUE;
    }

    /* Create driver object */
    gfxhidd = OOP_NewObject(NULL, gfxhiddname, NULL);
    D(bug("[LateGfxInit] gfxhidd 0x%p\n", gfxhidd));
    if (!gfxhidd)
        return FALSE;

    /* Attach system structures to it */
    mdd = driver_Setup(gfxhidd, GfxBase);
    D(bug("[LateGfxInit] monitor_driverdata 0x%p\n", mdd));
    if (!mdd) {
        OOP_DisposeObject(gfxhidd);
	return FALSE;
    }

    /* Create MonitorSpecs for the driver.
       Note that old specs will not be deleted!
       For now they will just stay laying around */
    ObtainSemaphore(GfxBase->MonitorListSemaphore);
    CreateMonitorSpecs(PrivGBase(GfxBase)->displays++, mdd, GfxBase);
    ReleaseSemaphore(GfxBase->MonitorListSemaphore);

    /* Poke the new driver into GfxBase (HACK!!!) */
    if (SDD(GfxBase)) {
        driver_Expunge(SDD(GfxBase), GfxBase);
        D(bug("[LateGfxInit] Old driver removed\n"));
    }
    D(bug("[LateGfxInit] Installing new driver\n"));
    SDD(GfxBase) = mdd;

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* LateGfxInit */
