/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Close a library.
    Lang: english
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_CloseLibrary
#   define DEBUG_CloseLibrary 0
#endif
#undef DEBUG
#if DEBUG_CloseLibrary
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */

	AROS_LH1(void, CloseLibrary,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, library, A1),

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    BPTR seglist;

    D(bug("CloseLibrary $%lx (\"%s\") by \"%s\"\n", library,
	library ? library->lib_Node.ln_Name : "(null)",
	SysBase->ThisTask->tc_Node.ln_Name));

    /* Something to do? */
    if(library!=NULL)
    {
	ASSERT_VALID_PTR(library);

	/* Single-thread the close routine. */
	Forbid();

	/* Do the close */
	seglist = AROS_LVO_CALL0(BPTR,struct Library *,library,2,);
	/*
	    Normally you'd expect the library to be expunged if this returns
	    non-zero, but this is only exec which doesn't know anything about
	    seglists - therefore dos.library has to SetFunction() into this
	    vector for the additional functionality.
	*/

	/* All done. */
	Permit();
    }
    else
    {
	/* Local vars not guaranteed to be initialised to 0. I initialise
	   it here to save an assignment in case the close went ok (common
	   path optimization). */
	seglist = 0;
    }

    AROS_COMPAT_SETDO(seglist);
    AROS_LIBFUNC_EXIT
} /* CloseLibrary */

