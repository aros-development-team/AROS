/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:16  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <dos/dos.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH1(void, RemLibrary,

/*  SYNOPSIS */
	__AROS_LA(struct Library *, library,A1),

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
    __AROS_FUNC_INIT

    /* Arbitrate for the library list */
    Forbid();

    /* Call expunge vector */
    (void)__AROS_LVO_CALL0(BPTR,3,library);
    /*
	Normally you'd expect the library to be expunged if this returns
	non-zero, but this is only exec which doesn't know anything about
	seglists - therefore dos.library has to SetFunction() into this
	vector for the additional functionality.
    */

    /* All done. */
    Permit();
    __AROS_FUNC_EXIT
} /* RemLibrary */

