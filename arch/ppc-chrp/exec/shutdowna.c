/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the operating system.
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/rtas.h>

/* See rom/kernel/issuper.c for documentation */

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


AROS_LH1(ULONG, ShutdownA,
    AROS_LHA(ULONG, action, D0),
    struct ExecBase *, SysBase, 173, Exec)
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
