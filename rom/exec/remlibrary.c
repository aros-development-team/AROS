/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a library from the list of public libraries.
    Lang: english
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "exec_debug.h"
#ifndef DEBUG_RemLibrary
#   define DEBUG_RemLibrary 0
#endif
#undef DEBUG
#if DEBUG_RemLibrary
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
#   define NATIVE(x)        x
#else
#   define NATIVE(x)        /* eps */
#endif

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

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    NATIVE(BPTR seglist;)

    D(bug("RemLibrary $%lx (\"%s\") by \"%s\"\n", library,
	library ? library->lib_Node.ln_Name : "(null)",
	SysBase->ThisTask->tc_Node.ln_Name));

    /* Arbitrate for the library list */
    Forbid();

    /* Call expunge vector */
    NATIVE(seglist =) AROS_LVO_CALL1(BPTR,
    	    	    	AROS_LCA(struct Library *, library, D0),
    	    	    	struct Library *,library,3,);
    /*
	Normally you'd expect the library to be expunged if this returns
	non-zero, but this is only exec which doesn't know anything about
	seglists - therefore dos.library has to SetFunction() into this
	vector for the additional functionality.
    */

    /* All done. */
    Permit();

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    /*
	Kludge to force the seglist to register d0. Ramlib patches this
	vector for seglist expunge capability and expects the seglist in
	d0 after it has called the original (this) function.
	Also see CloseDevice().
    */
    {
	/* Put the library base in register d0 */
	register BPTR ret __asm("d0") = seglist;

	/* Make sure the above assignment isn't optimized away */
	asm volatile("": : "r" (ret));
    }
#endif

    AROS_LIBFUNC_EXIT
} /* RemLibrary */

