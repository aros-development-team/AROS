/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Add a library to the public list of libraries.
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(void, AddLibrary,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, library,A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 66, Exec)

/*  FUNCTION
	Adds a given library to the system's library list after checksumming
	the library vectors. This function is not for general use but
	(of course) for building shared librarys that don't use exec's
	MakeLibrary() mechanism.

    INPUTS
	library - Pointer to a ready for use library structure.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemLibrary(), MakeLibrary(), MakeFunctions(), InitStruct(), SumLibrary().

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Just in case the user forgot them */
    library->lib_Node.ln_Type=NT_LIBRARY;
    library->lib_Flags|=LIBF_CHANGED;

    /* Build checksum for library vectors */
    SumLibrary(library);

    /* Arbitrate for the library list */
    Forbid();

    /* And add the library */
    Enqueue(&SysBase->LibList,&library->lib_Node);

    /* All done. */
    Permit();
    AROS_LIBFUNC_EXIT
} /* AddLibrary */

