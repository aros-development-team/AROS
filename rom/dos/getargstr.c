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

	__AROS_LH0(STRPTR, GetArgStr,

/*  SYNOPSIS */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 89, Dos)

/*  FUNCTION
	Returns a pointer to the argument string passed to the current
	process at startup.

    INPUTS

    RESULT
	Pointer to argument string.

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
    return me->pr_Arguments;
    __AROS_FUNC_EXIT
} /* GetArgStr */
