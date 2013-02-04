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
 * Hosted drivers are also responsible for registering own input
 * drivers if needed.
 */

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/driver.h>
#include <graphics/gfxbase.h>
#include <hidd/hidd.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>

#include "gdi.h"

static int gdi_Startup(struct gdiclbase *LIBBASE) 
{
    struct GfxBase *GfxBase;
    OOP_Object *kbd, *ms;
    OOP_Object *kbdriver = NULL;
    OOP_Object *msdriver = NULL;
    struct TagItem kbd_tags[] =
    {
        {aHidd_Name        , (IPTR)"GDIKbd"                    },
        {aHidd_HardwareName, (IPTR)"Windows GDI keyboard input"},
        {aHidd_ProducerName, (IPTR)"Microsoft Corp."           },
        {TAG_DONE          , 0                                 }
    };
    struct TagItem ms_tags[] =
    {
        {aHidd_Name        , (IPTR)"GDIMouse"                  },
        {aHidd_HardwareName, (IPTR)"Windows GDI mouse input"   },
        {aHidd_ProducerName, (IPTR)"Microsoft Corp."           },
        {TAG_DONE          , 0                                 }
    };

    D(bug("[GDI] gdi_Startup()\n"));

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    D(bug("[gdi_Startup] GfxBase 0x%p\n", GfxBase));
    if (!GfxBase)
        return FALSE;

    /* Add keyboard and mouse driver to the system */
    kbd = OOP_NewObject(NULL, CLID_HW_Kbd, NULL);
    ms  = OOP_NewObject(NULL, CLID_HW_Mouse, NULL);

    kbdriver = HW_AddDriver(kbd, LIBBASE->xsd.kbdclass, kbd_tags);
    if (kbdriver)
    {
	msdriver = HIDD_Mouse_AddHardwareDriver(ms, LIBBASE->xsd.mouseclass, ms_tags);
	if (!msdriver)
	    HIDD_Kbd_RemHardwareDriver(kbd, kbdriver);
    }

    /* If we got no input, we can't work, fail */
    if (!msdriver)
    {
	CloseLibrary(&GfxBase->LibNode);
        return FALSE;
    }

    /* We use ourselves, and noone else */
    LIBBASE->library.lib_OpenCnt = 1;

    /*
     * Now proceed to adding display modes. Install only one instance for the first time.
     * If needed, more displays are added by disk-based part.
     */
    AddDisplayDriver(LIBBASE->xsd.gfxclass, NULL, NULL);

    CloseLibrary(&GfxBase->LibNode);
    return TRUE;
}

/* This routine must be called AFTER everything all other initialization was run */
ADD2INITLIB(gdi_Startup, 10);
