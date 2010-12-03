/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: endnotify.c 34955 2010-10-25 23:32:59Z jmcmullan $

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include "dos_intern.h"
#include <exec/lists.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

#include <dos/notify.h>
#include <proto/dos.h>

#include <string.h>

	AROS_LH1(void, EndNotify,

/*  SYNOPSIS */
	AROS_LHA(struct NotifyRequest *, notify, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 149, Dos)

/*  FUNCTION

    End a notification (quit notifying for a request previously sent with
    StartNotify()).

    INPUTS

    notify  --  NotifyRequest used with StartNotify()

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    StartNotify()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[EndNotify] not implemented\n"));

    AROS_LIBFUNC_EXIT
} /* EndNotify */
