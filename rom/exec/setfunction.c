/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.11  1997/02/28 00:11:54  ldp
    AROSfA: Refine way to sign-extend from 16 to 32 bits

    Revision 1.10  1997/02/26 01:33:40  ldp
    AROSfA: added hack to keep dos.library patches working. See source for
    extensive comments.

    Revision 1.9  1997/01/01 03:46:16  ldp
    Committed Amiga native (support) code

    Changed clib to proto

    Revision 1.8  1996/12/10 13:51:54  aros
    Moved all #include's in the first column so makedepend can see it.

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
#include <aros/config.h>
#include <exec/execbase.h>
#include <aros/libcall.h>
#include <aros/machine.h>
#include <proto/exec.h>
#include "exec_debug.h"

#ifndef DEBUG_SetFunction
#   define DEBUG_SetFunction 0
#endif
#if DEBUG_SetFunction
#   undef DEBUG
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

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    APTR ret;

    /*
	Fix dos.library/ramlib attempts to SetFunction() CloseDevice/
	CloseLibrary/RemDevice/RemLibrary/OpenDevice/OpenLibrary.

	This also effectively limits the max offset to 32k, but this limit was
	already in the original, though not really documented.

	What happes is this: the prototype for the funcOffset says it is a
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
	dos.library.
    */
#if (AROS_FLAVOUR == AROS_FLAVOUR_NATIVE)
    if (funcOffset & 0x00008000)
    {
	funcOffset |= 0xffff0000;
    }
    else
    {
	funcOffset &= 0x0000ffff;
    }
#endif

    D(bug("SetFunction(%s, %lx, %lx) = ", (ULONG)library->lib_Node.ln_Name, funcOffset, (ULONG)newFunction));

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

#if 1
    __AROS_INITVEC (library, funcOffset);
#endif

    /* Write new one. */
    __AROS_SETVECADDR (library, funcOffset, newFunction);

    /* And clear the instruction cache. */
    CacheClearU();
#if 0
    /*
       Fixed to also flush data cache (very important for CopyBack style
       caches) [ldp]
    */
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

