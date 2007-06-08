/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the protection bits of a file.
    Lang: English
*/
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, SetProtection,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name,    D1),
	AROS_LHA(ULONG,  protect, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 31, Dos)

/*  FUNCTION

    INPUTS
	name	- name of the file
	protect - new protection bits

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
    InitIOFS(&iofs, FSA_SET_PROTECT, DOSBase);

    iofs.io_Union.io_SET_PROTECT.io_Protection = protect;

    return DoIOFS(&iofs, NULL, name, DOSBase) == 0;

    AROS_LIBFUNC_EXIT
} /* SetProtection */
