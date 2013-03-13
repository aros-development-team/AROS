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
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include LC_LIBDEFS_FILE

static int X11_Startup(LIBBASETYPEPTR LIBBASE) 
{
    struct TagItem kbd_tags[] =
    {
        {aHidd_Name        , (IPTR)"X11Kbd"                 },
        {aHidd_HardwareName, (IPTR)"X Window keyboard input"},
        {aHidd_ProducerName, (IPTR)"X.Org Foundation"       },
        {TAG_DONE          , 0                              }
    };
    struct TagItem mouse_tags[] =
    {
        {aHidd_Name        , (IPTR)"X11Mouse"              },
        {aHidd_HardwareName, (IPTR)"X Window pointer input"},
        {aHidd_ProducerName, (IPTR)"X.Org Foundation"      },
        {TAG_DONE          , 0                             }
    };
    struct GfxBase *GfxBase;
    OOP_Object *kbd, *ms;
    OOP_Object *kbdriver = NULL;
    OOP_Object *msdriver = NULL;
    int res = FALSE;
    ULONG err;

    D(bug("[X11] X11_Startup()\n"));

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    D(bug("[X11_Startup] GfxBase 0x%p\n", GfxBase));
    if (!GfxBase)
        return FALSE;

    /* Add keyboard and mouse driver to the system */
    kbd = OOP_NewObject(NULL, CLID_Hidd_Kbd, NULL);
    ms  = OOP_NewObject(NULL, CLID_Hidd_Mouse, NULL);

    if ((!kbd) || !(ms))
    {
        CloseLibrary(&GfxBase->LibNode);
        return FALSE;
    }	

    kbdriver = HW_AddDriver(kbd, LIBBASE->xsd.kbdclass, kbd_tags);
    if (kbdriver)
        msdriver = HW_AddDriver(ms, LIBBASE->xsd.mouseclass, mouse_tags);

    /* If we got no input, we can't work, fail */
    if (!msdriver)
    {
        CloseLibrary(&GfxBase->LibNode);
        return FALSE;
    }

    err = AddDisplayDriverA(LIBBASE->xsd.gfxclass, NULL, NULL);

    D(bug("[X11_Startup] AddDisplayDriver() result: %u\n", err));
    if (!err)
    {
        LIBBASE->library.lib_OpenCnt = 1;
        res = TRUE;
    }
    else
    {
	if (kbdriver)
	    HW_RemoveDriver(kbd, kbdriver);
	if (msdriver)
	    HW_RemoveDriver(ms, msdriver);
    }

    CloseLibrary(&GfxBase->LibNode);

    return res;
}

/* This routine must be called AFTER everything all other initialization was run */
ADD2INITLIB(X11_Startup, 10);
