/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a library.
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH2(struct Library *, OpenLibrary,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR,  libName, A1),
	AROS_LHA(ULONG,         version, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 92, Exec)

/*  FUNCTION
	Opens a library given by name and revision. If the library does not
	exist in the current system shared library list, the first the
	system ROMTag module list is tried. If this fails, and the DOS is
	running, then the library will be loaded from disk.

    INPUTS
	libName - Pointer to the library's name.
	version - the library's version number.

    RESULT
	Pointer to library structure or NULL.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	CloseLibrary().

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)
    struct Library * library;

    /* Arbitrate for the library list */
    Forbid();

    /* Look for the library in our list */
    library = (struct Library *) FindName (&SysBase->LibList, libName);

    /* Something found ? */
    if(library!=NULL)
    {
	/* Check version */
	if(library->lib_Version>=version)
	{
	    /* Call Open vector */
	    library=AROS_LVO_CALL1(struct Library *,
		AROS_LCA(ULONG,version,D0),
		struct Library *,library,1,lib
	    );
	}else
	    library=NULL;
    }

    /*
     *	We cannot handle loading libraries from disk. But this is taken
     *	care of by dos.library (well lddemon really) replacing this
     *	function with a SetFunction() call.
     */

    /* All done. */
    Permit();
    return library;
    AROS_LIBFUNC_EXIT
} /* OpenLibrary */
