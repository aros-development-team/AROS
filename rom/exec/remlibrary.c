/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/01 03:46:14  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.6  1996/12/10 13:51:51  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:55  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:06  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:16  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, RemLibrary,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, library,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 67, Exec)

/*  FUNCTION
	Calls the given library's expunge vector, thus trying to delete it.
	The library may refuse to do so and still be open after this call.

    INPUTS
	library - Pointer to the library structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddLibrary(), MakeLibrary(), MakeFunctions(), InitStruct(), SumLibrary().

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Arbitrate for the library list */
    Forbid();

    /* Call expunge vector */
    (void)AROS_LVO_CALL0(BPTR,struct Library *,library,3,);
    /*
	Normally you'd expect the library to be expunged if this returns
	non-zero, but this is only exec which doesn't know anything about
	seglists - therefore dos.library has to SetFunction() into this
	vector for the additional functionality.
    */

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* RemLibrary */

