/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH0(BPTR, Output,

/*  SYNOPSIS */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 10, Dos)

/*  FUNCTION
	Returns the current output stream or 0 if there is no current
	output stream.

    INPUTS

    RESULT
	Output stream handle.

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    
    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    return me->pr_COS;
    __AROS_FUNC_EXIT
} /* Output */
