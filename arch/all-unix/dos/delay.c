/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include "dos_intern.h"

/* Unix includes */
#include <unistd.h>
#include <errno.h>
#define timeval sys_timeval
#include <sys/time.h>
#undef timeval

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(void, Delay,

/*  SYNOPSIS */
	AROS_LHA(ULONG, timeout, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 33, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    struct sys_timeval tv;
    int ret;

    tv.tv_sec = 0;
    tv.tv_usec = timeout * 20000L;
    for (;;)
    {
	ret = select (0, NULL, NULL, NULL, &tv);
	if (ret == -1 && errno == EINTR)
	{
	    Disable();
	    SysBase->ThisTask->tc_State = TS_READY;
	    AddTail(&SysBase->TaskReady, &SysBase->ThisTask->tc_Node);
	    Enable();
	    Switch();
	}
	else
	    break;
    }

    AROS_LIBFUNC_EXIT
} /* Delay */
