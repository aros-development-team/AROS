/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Patch a library or device function
    Lang: english
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include "exec_debug.h"

#ifndef DEBUG_SetFunction
#   define DEBUG_SetFunction 0
#endif
#undef DEBUG
#if DEBUG_SetFunction
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

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
	On native builds, this contains a hack to fix dos.library/ramlib
	attempts to setfunction exec functions. Because of this, a funcOffset
	of more than 32 kB be truncated. This hack will also fix other programs
	only using the lower 16 bits of funcOffset and leaving garbage in the
	upper 16 bits. These programs should be fixed.

    SEE ALSO
	MakeLibrary(), MakeFunctions(), SumLibrary().

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    APTR ret;
#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    ULONG *vecaddr;
#endif

    D(bug("SetFunction(%s, %lx, %lx) = ", (ULONG)library->lib_Node.ln_Name, funcOffset, (ULONG)newFunction));

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    /*
	Fix dos.library/ramlib attempts to SetFunction() CloseDevice/
	CloseLibrary/RemDevice/RemLibrary/OpenDevice/OpenLibrary.

	This also effectively limits the max offset to 32k, but this limit was
	already in the original, though not really documented.

	What happens is this: the prototype for the funcOffset says it is a
	long, but the autodoc also says that only a0.w (lower 16 bits) is used.
	Dos.library/ramlib only sets the lower 16 bits of a0 to the required
	offset, without sign-extending to the upper 16 bits, in fact without
	even clearing them. These high 16 bits will therefore contain garbage:

	SetFunction(exec.library, 7804fe3e, fc6524) = 30303030 CloseDevice
	SetFunction(exec.library, 3030fe62, fc6528) = 30303030 CloseLibrary
	SetFunction(exec.library, 3030fe4a, fc651c) = 30303030 RemDevice
	SetFunction(exec.library, 3030fe6e, fc6520) = 30303030 RemLibrary
	SetFunction(exec.library, 3030fe44, fc6564) = 30303030 OpenDevice
	SetFunction(exec.library, 3030fdd8, fc659a) = 30303030 OpenLibrary

	In my [ldp] opinion, the autodoc should never have said that only A0.W
	is used for the funcOffset, while specifying a "long" in the prototype.
	This will stay broken and this fix will stay here until we fix
	dos.library/ramlib.
    */
    if (funcOffset & 0x00008000)
    {
	funcOffset |= 0xffff0000;
    }
    else
    {
	funcOffset &= 0x0000ffff;
    }

#else
    /* Vector pre-processing for non-native machines: */
    funcOffset = (-funcOffset) / LIB_VECTSIZE;

#endif

    /*
	Arbitrate for the jumptable. This isn't enough for interrupt callable
	functions - but it need not be.
    */
    Forbid();

    /* Mark the library as changed. */
    library->lib_Flags|=LIBF_CHANGED;

#if (AROS_FLAVOUR & AROS_FLAVOUR_NATIVE)
    /* The following section is coded like this (instead of using the macros),
       because else gcc will output 64-bit muls instructions, that are not
       present on the 68060 (and will crash it). It's faster this way, too. :) */
    vecaddr = (APTR)((ULONG)library + funcOffset);

    /* Get the old vector pointer */
    ret = (APTR)*(ULONG *)(((ULONG)vecaddr)+2);

    /* Set new vector and jmp instruction */
    *(UWORD *)vecaddr = 0x4ef9;
    *(ULONG *)(((ULONG)vecaddr)+2) = (ULONG)newFunction;

#else /* non-native section follows */
    /* Get old vector. */
    ret = __AROS_GETVECADDR (library, funcOffset);

    /* Don't forget to initialise the vector, or else there would be no actual
       assembler jump instruction in the vector */
    __AROS_INITVEC (library, funcOffset);

    /* Write new one. */
    __AROS_SETVECADDR (library, funcOffset, newFunction);

#endif /* end if system specific sections */

#if 1
    /* And clear the instruction cache. */
    /* Simply clear the entire cache... */
    CacheClearU();
#else
    /* ...or clear the vector address range specifically */
    CacheClearE (__AROS_GETJUMPVEC(library,funcOffset),LIB_VECTSIZE,CACRF_ClearI|CACRF_ClearD);
#endif

    /* Arbitration is no longer needed */
    Permit();

    /* Sum the library up again */
    SumLibrary(library);

    D(bug("%lx\n", ret));

    /* All done. */
    return ret;
    AROS_LIBFUNC_EXIT
} /* SetFunction */

