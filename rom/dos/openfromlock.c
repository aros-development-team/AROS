/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/12/09 13:53:35  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:33  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:49  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:55  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
#include <clib/dos_protos.h>

	AROS_LH1(BPTR, OpenFromLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 63, Dos)

/*  FUNCTION
	Convert a lock into a filehandle. If all went well the filehandle
	will be gone. In case of an error it must still be freed.

    INPUTS
	lock - Lock to convert.

    RESULT
	New filehandle or 0 in case of an error. IoErr() will give
	additional information in that case.

    NOTES
	Since locks and filehandles in AROS are identical this function
	is just the (useless) identity operator and thus can never fail.
	It's there for compatibility to Amiga OS.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /* Warning: Some very tricky operation ahead ;-). */
    return lock;
    AROS_LIBFUNC_EXIT
} /* OpenFromLock */
