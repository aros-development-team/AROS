/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:06  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <dos/dos.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, CloseLibrary,

/*  SYNOPSIS */
	__AROS_LA(struct Library *, library,A1),

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
    __AROS_FUNC_INIT

    /* Something to do? */
    if(library!=NULL)
    {
	/* Single-thread the close routine. */
	Forbid();

	/* Do the close */
	(void)__AROS_LVO_CALL0(BPTR,2,library);
	/*
	    Normally you'd expect the library to be expunged if this returns
	    non-zero, but this is only exec which doesn't know anything about
	    seglists - therefore dos.library has to SetFunction() into this
	    vector for the additional functionality.
	*/

	/* All done. */
	Permit();
    }

    __AROS_FUNC_EXIT
} /* CloseLibrary */

