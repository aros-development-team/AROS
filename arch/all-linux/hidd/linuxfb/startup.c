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
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <hidd/unixio_inline.h>

#include LC_LIBDEFS_FILE

/* Prevent redefinition of struct timeval */
#define timeval sys_timeval

#include <fcntl.h>

#undef timeval

#undef LSD
#define LSD(cl) (&LIBBASE->lsd)

static int LinuxFB_Startup(LIBBASETYPEPTR LIBBASE) 
{
    int res = FALSE;
    struct GfxBase *GfxBase;
    ULONG err;
    struct TagItem gfx_attrs[] =
    {
        {aHidd_LinuxFB_File, 0},
        {TAG_DONE          , 0}
    };

    D(bug("[LinuxFB] LinuxFB_Startup()\n"));

    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 41);
    D(bug("[LinuxFB_Startup] GfxBase 0x%p\n", GfxBase));
    if (!GfxBase)
        return FALSE;

    /* TODO: In future we will support more framebuffers */
    gfx_attrs[0].ti_Data = Hidd_UnixIO_OpenFile(LIBBASE->lsd.unixio, "/dev/fb0", O_RDWR, 0, NULL);
    if (gfx_attrs[0].ti_Data == -1)
    {
        CloseLibrary(&GfxBase->LibNode);
        return FALSE;
    }

    /*
     * In future we will be able to call this several times in a loop.
     * This will allow us to create several displays.
     */
    err = AddDisplayDriverA(LIBBASE->lsd.gfxclass, gfx_attrs, NULL);

    D(bug("[LinuxFB_Startup] AddDisplayDriver() result: %u\n", err));

    if (!err)
    {
        res = TRUE;
        /* We use ourselves, and noone else */
        LIBBASE->library.lib_OpenCnt = 1;
    }

    CloseLibrary(&GfxBase->LibNode);

    return res;
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
