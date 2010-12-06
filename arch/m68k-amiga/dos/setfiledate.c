/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: setfiledate.c 30792 2009-03-07 22:40:04Z neil $

    Desc: Change the date of a file.
    Lang: English
*/
#define DEBUG 0
#include <aros/debug.h>
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
	AROS_LHA(const struct DateStamp *, date, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 66, Dos)

/*  FUNCTION
	Change the modification time of a file or directory.

    INPUTS
	name - name of the file
	date - new file time

    RESULT
	Boolean success indicator. IoErr() gives additional information upon
	failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PacketHelperStruct phs;
    LONG status = DOSFALSE;

    D(bug("[SetFileDate] '%s' %x\n", name, date));

    if (getpacketinfo(DOSBase, name, &phs)) {
    	status = dopacket4(DOSBase, NULL, phs.port, ACTION_SET_DATE, (IPTR)NULL, phs.lock, phs.name, (IPTR)date);
    	freepacketinfo(DOSBase, &phs);
    }

    return status;

    AROS_LIBFUNC_EXIT
} /* SetFileDate */
