/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Retrieve the full pathname from a filehandle.
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

#include <aros/debug.h>

BOOL namefrom_internal(struct DosLibrary *DOSBase, BPTR lock, STRPTR buffer, LONG length, BOOL filehandle);

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH3(BOOL, NameFromFH,

/*  SYNOPSIS */
	AROS_LHA(BPTR,   lock,   D1),
	AROS_LHA(STRPTR, buffer, D2),
	AROS_LHA(LONG,   length, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 68, Dos)

/*
    FUNCTION
	Get the full path name associated with file-handle into a
	user supplied buffer.

    INPUTS
	fh     - File-handle to file or directory.
	buffer - Buffer to fill. Contains a NUL terminated string if
		 all went well.
	length - Size of the buffer in bytes.

    RESULT
	!=0 if all went well, 0 in case of an error. IoErr() will
	give additional information in that case.

*****************************************************************************/
/*AROS alias NameFromFH NameFromLock */
{
    AROS_LIBFUNC_INIT

    return namefrom_internal(DOSBase, lock, buffer, length, TRUE);
    
    AROS_LIBFUNC_EXIT
}
