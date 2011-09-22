/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Patch a library or device function
    Lang: english
*/

#include <aros/debug.h>
#include <exec/execbase.h>
#include <proto/intuition.h>
#include <aros/libcall.h>
#include <proto/exec.h>

#include "exec_debug.h"

/*****************************************************************************

    NAME */

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
		      bytes. It's the negative LVO (library vector offset)
		      multiplied with LIB_VECTSIZE.
	newFunction - New jumptable entry (pointer to the new function).

    RESULT
	Old jumptable entry (pointer to the old function).

    NOTES
	While it's more or less safe to patch a library vector with
	SetFunction() it's not possible to safely remove the patch later.
	So don't use this function if it can be avoided.

    EXAMPLE
	Patch of the function Open() from dos.library:
	You can find the LVO of 5 in clib/dos_protos.h.
	SetFunction(DOSBase, -5 * LIB_VECTSIZE, NewOpen);
	NewOpen must be prepared with AROS_UFH macros.

    BUGS
	None.

    SEE ALSO
	MakeLibrary(), MakeFunctions(), SumLibrary()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    APTR ret;

    DSETFUNCTION("SetFunction(%s, %ld, 0x%p)", library->lib_Node.ln_Name, funcOffset, newFunction);

    /* Vector pre-processing for non-native machines: */
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

    /* Don't forget to initialise the vector, or else there would be no actual
       assembler jump instruction in the vector */
    __AROS_INITVEC (library, funcOffset);

    /* Write new one. */
    __AROS_SETVECADDR (library, funcOffset, newFunction);

#ifdef __AROS_USE_FULLJMP
    /* And clear the instruction cache (only if vectors actually contain instructions) */
#if 1
    /*
     * Simply clear the entire cache...
     * CHECKME: Why? - sonic
     */
    CacheClearU();
#else
    /* ...or clear the vector address range specifically */
    CacheClearE (__AROS_GETJUMPVEC(library,funcOffset),LIB_VECTSIZE,CACRF_ClearI|CACRF_ClearD);
#endif
#endif

    /* Arbitration is no longer needed */
    Permit();

    /* Sum the library up again */
    SumLibrary(library);

    DSETFUNCTION("Old function: 0x%p", ret);

    /* All done. */
    return ret;
    AROS_LIBFUNC_EXIT
} /* SetFunction */

