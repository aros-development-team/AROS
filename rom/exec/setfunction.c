/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1996/10/24 15:36:47  aros
    Named all macros which the user/developer can use as "AROS".

    For some strange reason, GCC produces incorrect code for "x /= -5;". "x = (-x)
    / 5" works...

    Revision 1.6  1996/10/23 14:28:54  aros
    Use the respective macros to access and manipulate a libraries' jumptable

    Revision 1.5  1996/10/19 17:07:27  aros
    Include <aros/machine.h> instead of machine.h

    Revision 1.4  1996/08/13 13:56:08  digulla
    Replaced AROS_LA by AROS_LHA
    Replaced some AROS_LH*I by AROS_LH*
    Sorted and added includes

    Revision 1.3  1996/08/01 17:41:19  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/machine.h>

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	AROS_LH3(APTR, SetFunction,

/*  SYNOPSIS */
	AROS_LHA(struct Library *, library,     A1),
	AROS_LHA(LONG,             funcOffset,  A0),
	AROS_LHA(APTR,             newFunction, D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 70, Exec)

/*  FUNCTION
	Replaces a certain jumptable entry with another one. This function only
	Forbid()s taskswitching but doesn't Disable() interrupts. You have
	to do your own arbitration for functions which are callable from
	interrupts.

    INPUTS
	library     - Pointer to library structure.
	funcOffset  - Offset of the jumpvector from the library base address in
		      bytes.
	newFunction - New jumptable entry (pointer to the new function).

    RESULT
	Old jumptable entry (pointer to the old function).

    NOTES
	While it's more or less safe to patch a library vector with
	SetFunction() it's not possible to safely remove the patch later.
	So don't use this function if it can be avoided.

    EXAMPLE

    BUGS

    SEE ALSO
	MakeLibrary(), MakeFunctions(), SumLibrary().

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    APTR ret;

    funcOffset = (-funcOffset) / LIB_VECTSIZE;

    /*
	Arbitrate for the jumptable. This isn't enough for interrupt callable
	functions - but it need not be.
    */
    Forbid();

    /* Mark the library as changed. */
    library->lib_Flags|=LIBF_CHANGED;

    /* Get old vector. */
    ret = __AROS_GETVECADDR (library, funcOffset);

    /* Write new one. */
    __AROS_SETVECADDR (library, funcOffset, newFunction);

    /* And clear the instructiuon cache. */
    CacheClearE (__AROS_GETJUMPVEC(library,funcOffset),LIB_VECTSIZE,CACRF_ClearI);

    /* Arbitration is no longer needed */
    Permit();

    /* Sum the library up again */
    SumLibrary(library);

    /* All done. */
    return ret;
    AROS_LIBFUNC_EXIT
} /* SetFunction */

