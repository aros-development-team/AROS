/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:27:13  digulla
    Added copyright notics and made headers conform

    Desc:
    Lang: english
*/
#include <aros/libcall.h>

/*****************************************************************************

    NAME */
	#include <exec/libraries.h>
	#include <clib/exec_protos.h>

	__AROS_LH1(struct Library *, OldOpenLibrary,

/*  SYNOPSIS */
	__AROS_LA(UBYTE *, libName, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 68, Exec)

/*  FUNCTION
	This is the same function as OpenLibrary(), only that it uses 0 as
	version number. This function is obsolete. Don't use it.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenLibrary().

    INTERNALS

    HISTORY

*****************************************************************************/
{
    __AROS_FUNC_INIT
    return OpenLibrary(libName,0);
    __AROS_FUNC_EXIT
} /* OldOpenLibrary */
