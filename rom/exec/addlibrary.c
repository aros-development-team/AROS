/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.6  1996/12/10 13:51:34  aros
    Moved all #include's in the first column so makedepend can see it.

    Revision 1.5  1996/10/24 15:50:41  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:55:55  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:02  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
#include <clib/exec_protos.h>

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

