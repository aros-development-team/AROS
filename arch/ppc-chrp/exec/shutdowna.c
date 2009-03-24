/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id: coldreboot.c 18441 2003-07-07 20:01:00Z hkiel $

    Desc: ShutdownA() - Shut down the operating system.
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/rtas.h>

static int rtas_call(struct ExecBase *SysBase, const char *method, int nargs, int nret, void *output, ...)
{
	va_list args;
	void *RTASBase = OpenResource("rtas.resource");
	int retval;

	va_start(args, output);
	retval = RTASCall(method, nargs, nret, output, args);
	va_end(args);

	return retval;
}

/*****************************************************************************

    NAME */

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

    void *RTASBase = OpenResource("rtas.resource");

    if (RTASBase)
    {
    	if (action == SD_ACTION_COLDREBOOT)
    		rtas_call(SysBase, "system-reboot", 0, 1, NULL);
    	else if (action ==SD_ACTION_POWEROFF)
    		rtas_call(SysBase, "power-off", 2, 1, NULL, -1, -1);
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
