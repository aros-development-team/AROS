/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:41  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:46  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:59  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:07  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <dos/dos.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <clib/exec_protos.h>

	AROS_LH1(void, CloseLibrary,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, library,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 69, Exec)

/*  FUNCTION
	Closes a previously opened library. It is legal to call this function
	with a NULL pointer.

    INPUTS
	library - Pointer to library structure or NULL.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenLibrary().

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Something to do? */
    if(library!=NULL)
    {
	/* Single-thread the close routine. */
	Forbid();

	/* Do the close */
	(void)AROS_LVO_CALL0(BPTR,struct Library,library,2,);
	/*
	    Normally you'd expect the library to be expunged if this returns
	    non-zero, but this is only exec which doesn't know anything about
	    seglists - therefore dos.library has to SetFunction() into this
	    vector for the additional functionality.
	*/

	/* All done. */
	Permit();
    }

    AROS_LIBFUNC_EXIT
} /* CloseLibrary */

