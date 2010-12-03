/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: startnotify.c 34705 2010-10-13 20:30:16Z jmcmullan $

    Desc:
    Lang: English
*/
#include <dos/notify.h>
#include <dos/exall.h>
#include <proto/dos.h>
#include "dos_intern.h"
#include <aros/debug.h>
#include <string.h>

/*****************************************************************************

    NAME */

	AROS_LH1(BOOL, StartNotify,

/*  SYNOPSIS */
	AROS_LHA(struct NotifyRequest *, notify, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 148, Dos)

/*  FUNCTION

    Send a notification request to a filesystem. You will then be notified
    whenever the file (or directory) changes.

    INPUTS

    notify  --  a notification request for the file or directory to monitor

    RESULT

    Success/failure indicator.

    NOTES

    The file or directory connected to a notification request does not have
    to exist at the time of calling StartNotify().
    The NotifyRequest used with this function should not be altered while
    active.

    EXAMPLE

    BUGS

    SEE ALSO

    EndNotify(), <dos/notify.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[StartNotify] not implemented\n"));
    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* StartNotify */
