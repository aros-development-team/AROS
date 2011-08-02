/*
 * A newstyle startup code for resident display drivers.
 *
 * Now it's the job of the driver to add ifself to the system.
 * The driver does not have to be a library anymore, it can be
 * plain executable which is started from DEVS:Monitors.
 *
 * The job of driver startup code is to create all necessary
 * classes (driver class and bitmap class) and create as many
 * driver objects as possible. Every object needs to be given
 * to AddDisplayDriverA() in order to become functional.
 *
 * When the transition completes, kludges from bootmenu and
 * dosboot will be removed. Noone will care about library
 * open etc.
 */

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

static int LinuxFB_Startup(LIBBASETYPEPTR LIBBASE) 
{
    struct GfxBase *GfxBase;
    OOP_Object *gfxhidd;
    OOP_Object *kbd, *ms;
    OOP_Object *kbdriver;
    OOP_Object *msdriver = NULL;

    D(bug("[LinuxFB] LinuxFB_Startup()\n"));

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    D(bug("[LinuxFB_Startup] GfxBase 0x%p\n", GfxBase));
    if (!GfxBase)
        return FALSE;

    /* Add keyboard and mouse driver to the system */
    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    if (kbd) {
        ms = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);
	if (ms) {
            kbdriver = HIDD_Kbd_AddHardwareDriver(kbd, LIBBASE->lsd.kbdclass, NULL);
	    if (kbdriver) {
		msdriver = HIDD_Mouse_AddHardwareDriver(ms, LIBBASE->lsd.mouseclass, NULL);
		if (!msdriver)
		    HIDD_Kbd_RemHardwareDriver(kbd, kbdriver);
	    }
	    OOP_DisposeObject(ms);
	}    
	OOP_DisposeObject(kbd);
    }

    /* If we got no input, we can't work, fail */
    if (!msdriver) {
	CloseLibrary(&GfxBase->LibNode);
        return FALSE;
    }

    /* We use ourselves, and noone else */
    LIBBASE->library.lib_OpenCnt = 1;

    /* In future we will be able to call this several times in a loop.
       This will allow us to create several displays. */
    gfxhidd = OOP_NewObject(LIBBASE->lsd.gfxclass, NULL, NULL);
    D(bug("[LinuxFB_Startup] gfxhidd 0x%p\n", gfxhidd));

    if (gfxhidd) {
        ULONG err = AddDisplayDriverA(gfxhidd, NULL);

	D(bug("[LinuxFB_Startup] AddDisplayDriver() result: %u\n", err));
	if (err) {
	    OOP_DisposeObject(gfxhidd);
	    gfxhidd = NULL;
	}
    }

    CloseLibrary(&GfxBase->LibNode);

    /* We always return TRUE because we added
       keyboard and mouse drivers */
    return TRUE;
}

static int LinuxFB_Test(LIBBASETYPEPTR LIBBASE)
{
    OOP_Class *x11 = OOP_FindClass("hidd.gfx.x11");

    D(bug("[LinuxFB_Test] X11 class 0x%p\n", x11));
    return x11 ? FALSE : TRUE;
}

/* This routine must be called AFTER everything all other initialization was run */
ADD2INITLIB(LinuxFB_Startup, 10);

ADD2INITLIB(LinuxFB_Test, -10);
