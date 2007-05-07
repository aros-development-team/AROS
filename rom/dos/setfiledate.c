/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the date of a file.
    Lang: English
*/
#include <string.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>
#include <dos/dos.h>

	AROS_LH2(BOOL, SetFileDate,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR,       name, D1),
	AROS_LHA(struct DateStamp *, date, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 66, Dos)

/*  FUNCTION
	Change the modification time of a file or directory.

    INPUTS
	name - name of the file
	date - new file time

    RESULT
	!= 0 if all went well, 0 else. IoErr() gives additional
	information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Get pointer to I/O request. Use stackspace for now. */
    struct IOFileSys iofs;

    /* Prepare I/O request. */
    InitIOFS(&iofs, FSA_SET_DATE, DOSBase);

    memcpy(&iofs.io_Union.io_SET_DATE.io_Date, date, sizeof(struct DateStamp));

    return !DoName(&iofs, name, DOSBase);

    AROS_LIBFUNC_EXIT
} /* SetFileDate */
