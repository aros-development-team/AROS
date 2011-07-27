/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the operating system.
    Lang: english
*/

#include <asm/io.h>
#include <exec/tasks.h>
#include <hardware/efi/runtime.h>
#include <proto/dos.h>

#include "exec_util.h"

/*****************************************************************************

    NAME */
#include <proto/exec.h>

	AROS_LH1(ULONG, ShutdownA,

/*  SYNOPSIS */
	AROS_LHA(ULONG, action, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 173, Exec)

/*  FUNCTION
	This function will shut down the operating system.

    INPUTS
	action - what to do:
	 * SD_ACTION_POWEROFF   - power off the machine.
	 * SD_ACTION_COLDREBOOT - cold reboot the machine (not only AROS).

    RESULT
	This function does not return in case of success. Otherwise it returns
	zero.

    NOTES
	It can be quite harmful to call this function. It may be possible that
	you will lose data from other tasks not having saved, or disk buffers
	not being flushed. Plus you could annoy the (other) users.

    EXAMPLE

    BUGS

    SEE ALSO
	ColdReboot()

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (PD(SysBase).efiRT)
    {
    	/*
    	 * If the system has EFI firmware, use its runtime interface.
    	 * Port 0xFE may not work on such machines (Mac).
    	 */
    	IPTR efiAction;

    	switch (action)
    	{
    	case SD_ACTION_COLDREBOOT:
    	    efiAction = EFI_Reset_Cold;
    	    break;

    	case SD_ACTION_POWEROFF:
    	    efiAction = EFI_Reset_Shutdown;
    	    break;

    	default:
    	    /* Unknown action */
    	    return 0;
    	}

    	PD(SysBase).efiRT->ResetSystem(efiAction, 0, 0, NULL);
    }
    else
    {
    	/* No EFI, poor-man fallback */
    	if (action == SD_ACTION_COLDREBOOT)
    	{
    	    struct DosLibrary *DOSBase;

    	    /*
    	     * Don't call reset callbacks because their action is not
    	     * recoverable.
    	     * On IntelMac port 0xFE doesn't work, so the function should
    	     * be able to return cleanly.
    	     * TODO: Implement alternative reset for Mac (ACPI ?)
    	     *
            Exec_DoResetCallbacks((struct IntExecBase *)SysBase); */
            outb(0xFE, 0x64);

	    /*
	     * Keyboard controller can be slow, so we need to wait for some time.
	     * If we don't do this, we'll can see "Unsupported action" error, immediately
	     * followed by a restart, which looks strange.
	     * we use dos.library/Delay() here for simplicity.
	     */
	    DOSBase = (APTR)OpenLibrary("dos.library", 36);
	    if (DOSBase)
	    {
	    	Delay(50);
	    	CloseLibrary((struct Library *)DOSBase);
	    }
	}
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
