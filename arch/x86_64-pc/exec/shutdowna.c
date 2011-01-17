/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the operating system.
    Lang: english
*/

#include <asm/io.h>
#include <exec/tasks.h>

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
	This function does not return in case of success. Otherwise is returns
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

    switch (action)
    {
	    case SD_ACTION_COLDREBOOT:
		    outb(0xFE, 0x64);
		    break;

	    case SD_ACTION_POWEROFF:
		    /*
			    Just wait till the power button is pressed
			    (or until we can do it via acpi ;))
		    */
		    asm("hlt");
		    break;

	    default:
		    break;
    }
    return 0;

    AROS_LIBFUNC_EXIT
}
