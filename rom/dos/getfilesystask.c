/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get the filesystem handler for a process.
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH0(struct MsgPort *, GetFileSysTask,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 87, Dos)

/*  FUNCTION
	Return the default filesystem handler for this process.

    INPUTS
	None.

    RESULT
	The default filesystem handler for this process.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SetFileSysTask()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    return BADDR(((struct Process *)FindTask(NULL))->pr_FileSystemTask);

    AROS_LIBFUNC_EXIT
} /* GetFileSysTask */
