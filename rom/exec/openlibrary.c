/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:53  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:56:05  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:15  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/lists.h>
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/libraries.h>
	#include <clib/exec_protos.h>

	AROS_LH2(struct Library *, OpenLibrary,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, libName, A1),
	AROS_LHA(ULONG,   version, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 92, Exec)

/*  FUNCTION
	Opens a library given by name and revision. If the library
	doesn't exist in memory it is tried to load it from disk.
	It this fails too, NULL is returned.

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

    HISTORY
	21-10-95    digulla automatically created from
			    include:clib/exec_protos.h

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
		struct Library *,library,1,
	    );
	}else
	    library=NULL;
    }
    /*
	else
	{
	Under normal circumstances you'd expect the library loading here -
	but this is only exec which doesn't know anything about the
	filesystem level. Therefore dos.library has to SetFunction() this vector
	for the additional functionality.
	}
    */

    /* All done. */
    Permit();
    return library;
    AROS_LIBFUNC_EXIT
} /* OpenLibrary */
