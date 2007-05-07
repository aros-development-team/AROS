/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Set the owner of a file.
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

	AROS_LH2(BOOL, SetOwner,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name,       D1),
	AROS_LHA(ULONG,  owner_info, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 166, Dos)

/*  FUNCTION

    INPUTS
	name	    --  name of the file
	owner_info  --  (UID << 16) + GID

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
    InitIOFS(&iofs, FSA_SET_OWNER, DOSBase);

    iofs.io_Union.io_SET_OWNER.io_UID = owner_info >> 16;
    iofs.io_Union.io_SET_OWNER.io_GID = owner_info & 0xffff;

    return !DoName(&iofs, name, DOSBase);

    AROS_LIBFUNC_EXIT
} /* SetOwner */
