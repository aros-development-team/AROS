/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
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

	__AROS_LH2(struct Library *, OpenLibrary,

/*  SYNOPSIS */
	__AROS_LA(UBYTE *, libName, A1),
	__AROS_LA(ULONG,   version, D0),

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
    __AROS_FUNC_INIT

    __AROS_BASE_EXT_DECL(struct ExecBase *,SysBase)
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
	    library=__AROS_LVO_CALL1(struct Library *,1,library,version,D0);
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
    __AROS_FUNC_EXIT
} /* OpenLibrary */
