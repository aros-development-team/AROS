/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ShutdownA() - Shut down the operating system.
    Lang: english
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_util.h"

/* See rom/exec/shutdowna.c for documentation */

AROS_LH1(ULONG, ShutdownA,
    AROS_LHA(ULONG, action, D0),
    struct ExecBase *, SysBase, 173, Exec)
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
