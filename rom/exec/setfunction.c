/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/01 17:41:19  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <exec/execbase.h>
#include <aros/libcall.h>
#include "machine.h"

/*****************************************************************************

    NAME */
	#include <clib/exec_protos.h>

	__AROS_LH3(APTR, SetFunction,

/*  SYNOPSIS */
	__AROS_LA(struct Library *, library,     A1),
	__AROS_LA(LONG,             funcOffset,  A0),
	__AROS_LA(APTR,             newFunction, D0),

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
    __AROS_FUNC_INIT

    APTR ret;

    /*
	Arbitrate for the jumptable. This isn't enough for interrupt callable
	functions - but it need not be.
    */
    Forbid();

    /* Mark the library as changed. */
    library->lib_Flags|=LIBF_CHANGED;

    /* Get old vector. */
    ret=GET_VEC((struct JumpVec *)((char *)library+funcOffset));

    /* Write new one. */
    SET_VEC((struct JumpVec *)((char *)library+funcOffset),newFunction);

    /* And clear the instructiuon cache. */
    CacheClearE((char *)library+funcOffset,LIB_VECTSIZE,CACRF_ClearI);

    /* Arbitration is no longer needed */
    Permit();

    /* Sum the library up again */
    SumLibrary(library);

    /* All done. */
    return ret;
    __AROS_FUNC_EXIT
} /* SetFunction */

