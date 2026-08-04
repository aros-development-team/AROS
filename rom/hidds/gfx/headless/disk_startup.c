/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Disk-resident loader for the headless display driver.
*/

#include <aros/debug.h>

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

/*
 * The driver adds itself to graphics.library from its own library init
 * (see headlessgfx_init.c), so all this has to do is bring the library
 * in. A driver kept on disk gets no other chance to run: AROSMonDrvs
 * executes what it finds in DEVS:Monitors, and that is what this is.
 */

int __nocommandline = 1;

int main(void)
{
    struct Library *HeadlessGfxBase;

    HeadlessGfxBase = OpenLibrary("headlessgfx.hidd", 0);
    if (HeadlessGfxBase == NULL)
        HeadlessGfxBase = OpenLibrary("DEVS:Drivers/headlessgfx.hidd", 0);

    if (HeadlessGfxBase == NULL)
    {
        D(bug("[HeadlessGfx:Disk] headlessgfx.hidd would not open\n"));
        return RETURN_FAIL;
    }

    D(bug("[HeadlessGfx:Disk] driver loaded @ 0x%p\n", HeadlessGfxBase));

    /*
     * Left open on purpose - closing the last reference would expunge
     * the driver that has just been registered.
     */
    return RETURN_OK;
}
