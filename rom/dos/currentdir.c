/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:48  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(BPTR, CurrentDir,

/*  SYNOPSIS */
	__AROS_LA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 21, Dos)

/*  FUNCTION
	Sets a new directory as the current directory. Returns the old one.
	0 is valid in both cases and represents the boot filesystem.

    INPUTS
	lock - Lock for the new current directory.

    RESULT
	Old current directory.

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
    BPTR old;

    /* Nothing spectacular */
    old=me->pr_CurrentDir;
    me->pr_CurrentDir=lock;
    return old;
    __AROS_FUNC_EXIT
} /* CurrentDir */
