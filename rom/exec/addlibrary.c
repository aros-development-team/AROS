/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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
	Some old Amiga software expects that AddLibrary returns the
	library which was just added. When in binary compatibility mode
	AROS does this too.

    EXAMPLE

    BUGS

    SEE ALSO
	RemLibrary(), MakeLibrary(), MakeFunctions(), InitStruct(), SumLibrary().

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    ASSERT_VALID_PTR(library);

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
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    /*
	Kludge to force the library to register d0.
	Some libraries seem to use this side effect in
	their init code. 
    */
    {
        /* Put the library base in register d0 */
        register struct Library *ret __asm("d0") = library;

        /* Make sure the above assignment isn't optimized away */
        __asm __volatile("": : "r" (ret));
    }
#endif

    AROS_LIBFUNC_EXIT
} /* AddLibrary */
