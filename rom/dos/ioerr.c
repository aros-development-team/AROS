/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.7  1998/10/20 16:44:43  hkiel
    Amiga Research OS

    Revision 1.6  1997/01/27 00:36:23  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:32  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:30  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:47  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:52  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH0(LONG, IoErr,

/*  SYNOPSIS */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 22, Dos)

/*  FUNCTION
	Get the dos error code for the current process.

    INPUTS

    RESULT
	Error code.

    NOTES

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

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    return me->pr_Result2;
    AROS_LIBFUNC_EXIT
} /* IoErr */
