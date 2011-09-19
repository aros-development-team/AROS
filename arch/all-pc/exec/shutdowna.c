/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the machine, PC version.
    Lang: english
*/

#include <asm/io.h>
#include <proto/dos.h>
#include <proto/exec.h>

/*
 * This code performs machine reset via legacy PC keyboard controller hardware.
 * On modern machines it's replaced by either efi.resource or acpi.resource,
 * using SetFunction().
 */
AROS_LH1(ULONG, ShutdownA,
	 AROS_LHA(ULONG, action, D0),
	 struct ExecBase *, SysBase, 173, Exec)
{
    AROS_LIBFUNC_INIT

    /* Nothing worked, poor-man fallback */
    if (action == SD_ACTION_COLDREBOOT)
    {
    	struct DosLibrary *DOSBase;

        outb(0xFE, 0x64);
	/*
	 * Keyboard controller can be slow, so we need to wait for some time.
	 * If we don't do this, we'll can see "Unsupported action" error, immediately
	 * followed by a restart, which looks strange.
	 * we use dos.library/Delay() here for simplicity. It works from within tasks.
	 */
	DOSBase = (APTR)OpenLibrary("dos.library", 36);
	if (DOSBase)
	{
	    Delay(50);
	    CloseLibrary((struct Library *)DOSBase);
	}
	/*
	 * On some machines (new PCs without PS/2 keyboard and ACPI disabled)
	 * this might not work.
	 * So we need to be able to return cleanly.
	 */
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
