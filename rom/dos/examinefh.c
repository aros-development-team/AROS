/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function ExamineFH().
    Lang: English
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, ExamineFH,

/*  SYNOPSIS */
	AROS_LHA(BPTR,                   handle, D1),
	AROS_LHA(struct FileInfoBlock *, fib,    D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 65, Dos)
/*
    FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
*/
{
    AROS_LIBFUNC_INIT

    return Examine(handle, fib);

    AROS_LIBFUNC_EXIT
} /* ExamineFH */
