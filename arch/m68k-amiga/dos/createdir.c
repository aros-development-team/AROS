/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: createdir.c 30792 2009-03-07 22:40:04Z neil $

    Desc: Create a new directory.
    Lang: English
*/
#define DEBUG 0
#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, CreateDir,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 20, Dos)

/*  FUNCTION
	Creates a new directory under the given name. If all went well, an
	exclusive lock on the new diretory is returned.

    INPUTS
	name  -- NUL terminated name.

    RESULT
	Exclusive lock to the new directory or 0 if it couldn't be created.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR lock = BNULL;
    struct PacketHelperStruct phs;
    LONG error;

    D(bug("[CreateDir] '%s'\n", name));

    if (getpacketinfo(DOSBase, name, &phs)) {
    	lock = dopacket2(DOSBase, &error, phs.port, ACTION_CREATE_DIR, phs.lock, phs.name);
    	freepacketinfo(DOSBase, &phs);
    }

    return lock;

    AROS_LIBFUNC_EXIT
} /* CreateDir */

