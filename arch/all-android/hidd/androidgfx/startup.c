/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * A newstyle startup code for resident display drivers.
 *
 * Now it's the job of the driver to add ifself to the system.
 * The driver does not have to be a library anymore, it can be
 * plain executable which is started from DEVS:Monitors.
 *
 * The job of driver startup code is to create all necessary
 * classes (driver class and bitmap class) and create as many
 * driver objects as possible. Display driver objects are created
 * by AddDisplayDriverA() after some checks.
 *
 * We are hosted, so we register also own input drivers.
 */

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/gfxbase.h>
#include <graphics/driver.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

static int agfx_Startup(LIBBASETYPEPTR LIBBASE) 
{
    struct GfxBase *GfxBase;
    OOP_Object *kbd, *ms;
    OOP_Object *kbdriver;
    OOP_Object *msdriver = NULL;

    D(bug("[AGFX.Startup] agfx_Startup()\n"));

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    D(bug("[AGFX.Startup] GfxBase 0x%p\n", GfxBase));
    if (!GfxBase)
        return FALSE;

    /* Add keyboard and mouse driver to the system */
    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    if (kbd)
    {
        ms = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);
	if (ms)
	{
            kbdriver = HIDD_Kbd_AddHardwareDriver(kbd, LIBBASE->xsd.kbdclass, NULL);
	    if (kbdriver)
	    {
		msdriver = HIDD_Mouse_AddHardwareDriver(ms, LIBBASE->xsd.mouseclass, NULL);
		if (!msdriver)
		    HIDD_Kbd_RemHardwareDriver(kbd, kbdriver);
	    }
	    OOP_DisposeObject(ms);
	}
	OOP_DisposeObject(kbd);
    }

    /* If we got no input, we can't work, fail */
    if (!msdriver)
    {
	CloseLibrary(&GfxBase->LibNode);
        return FALSE;
    }

    /* We use ourselves, and noone else */
    LIBBASE->library.lib_OpenCnt = 1;

    /* Install the driver. At the moment we have only one display. */
    AddDisplayDriverA(LIBBASE->xsd.gfxclass, NULL, NULL);

    CloseLibrary(&GfxBase->LibNode);
    return TRUE;
}

/* This routine must be called AFTER everything all other initialization was run */
ADD2INITLIB(agfx_Startup, 10);
