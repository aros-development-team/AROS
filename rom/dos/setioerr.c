/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:23  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(LONG, SetIoErr,

/*  SYNOPSIS */
	__AROS_LA(LONG, result, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 77, Dos)

/*  FUNCTION
	Sets to dos error code for the current process.

    INPUTS
	result - new error code

    RESULT
	Old error code.

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
    
    /* old contents */
    LONG old;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    old=me->pr_Result2;
    me->pr_Result2=result;
    return old;
    __AROS_FUNC_EXIT
} /* SetIoErr */
