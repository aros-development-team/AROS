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

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

static int gdi_Startup(LIBBASETYPEPTR LIBBASE) 
{
    struct GfxBase *GfxBase;
    OOP_Object *gfxhidd;

    D(bug("[GDI] gdi_Startup()\n"));

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    D(bug("[gdi_Startup] GfxBase 0x%p\n", GfxBase));
    if (!GfxBase)
        return FALSE;

    /* We use ourselves, and noone else */
    LIBBASE->library.lib_OpenCnt = 1;

    /* In future we will be able to call this several times in a loop.
       This will allow us to create several displays. */
    gfxhidd = OOP_NewObject(LIBBASE->xsd.gfxclass, NULL, NULL);
    D(bug("[gdi_Startup] gfxhidd 0x%p\n", gfxhidd));

    if (gfxhidd) {
        ULONG err = AddDisplayDriverA(gfxhidd, NULL);

	D(bug("[gdi_Startup] AddDisplayDriver() result: %u\n", err));
	if (err) {
	    OOP_DisposeObject(gfxhidd);
	} else
	    return TRUE;
    }

    CloseLibrary(&GfxBase->LibNode);
    
    /* FIXME: if we return FALSE here, expunge will be caused.
       This means that DLL containing keyboard hook will be unloaded,
       which means crash as soon as any key pressed. */
    return TRUE;
}

/* This routine must be called AFTER everything all other initialization was run */
ADD2INITLIB(gdi_Startup, 10);
