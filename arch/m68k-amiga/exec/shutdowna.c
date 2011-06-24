/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the operating system.
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_util.h"

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

    switch (action) {
    case SD_ACTION_POWEROFF:
    {
    	/* No stock Amiga hardware is known to support this.
    	 * Emulation will support it (unless high compatibility setting enabled). */
    	typedef ULONG (*UAELIBFUNC)(ULONG);
    	UAELIBFUNC uaelibfunc = NULL;
	APTR uaeres;
	
	uaeres = OpenResource("uae.resource");
	if (uaeres) { /* new method that allows dynamic UAE ROM location */
	    uaelibfunc = AROS_LVO_CALL1(APTR,
		AROS_LCA(UBYTE*, "uaelib_demux", A0),
		APTR, uaeres, 1, );
	}
	if (uaelibfunc == NULL) {
	    /* old-style absolute address */
	    uaelibfunc = (UAELIBFUNC)(0x00F00000 + 0xFF60);
	    if ((((ULONG*)uaelibfunc)[0] & 0xff00ffff) != 0xa0004e75)
	    	uaelibfunc = NULL;
	}
	if (uaelibfunc) {
	    uaelibfunc(13);
	    /* This may return. Quits when next vblank arrives */
	    for(;;)
		;
	}
    	break;
    }
    case SD_ACTION_COLDREBOOT:
    	ColdReboot();
    	break;
    }

    return -1;

    AROS_LIBFUNC_EXIT
}
