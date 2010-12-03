/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id: errorreport.c 33168 2010-05-04 06:17:31Z sonic $

    Desc:
    Lang: English
*/

#include "dos_intern.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <intuition/intuition.h>
#include <exec/ports.h>

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(BOOL, ErrorReport,

/*  SYNOPSIS */
	AROS_LHA(LONG            , code  , D1),
	AROS_LHA(LONG            , type  , D2),
	AROS_LHA(IPTR            , arg1  , D3),
	AROS_LHA(struct MsgPort *, device, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 80, Dos)

/*  FUNCTION
    Displays a requester with Retry/Cancel buttons for an error.
    IoErr() is set to "code".
	
    INPUTS

    code    --  The error to put up the requester for
    type    --  Type of request

                REPORT_LOCK    --  arg1 is a lock (BPTR).
                REPORT_FH      --  arg1 is a filehandle (BPTR).
		REPORT_VOLUME  --  arg1 is a volumenode (C pointer).
		REPORT_INSERT  --  arg1 is the string for the volumename

    arg1    --  Argument according to type (see above)
    device  --  Optional handler task address (obsolete!)

    RESULT
    DOSFALSE - user has selected "Retry"
    DOSTRUE  - user has selected "Cancel" or code wasn't understood or
	       pr_WindowPtr is -1 or if an attempt to open the requester fails.

    NOTES

    Locks and filehandles are the same in AROS so there is redundancy in
    the parameters. Furthermore, the 'device' argument is not cared about
    as AROS doesn't build filesystems with handlers.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return TRUE;
 
    AROS_LIBFUNC_EXIT
} /* ErrorReport */
