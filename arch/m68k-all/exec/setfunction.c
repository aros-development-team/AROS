/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Patch a library or device function
    Lang: english
*/
#include <aros/config.h>
#include <exec/execbase.h>
#include <proto/intuition.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <exec/rawfmt.h>

#include "exec_debug.h"
#include "exec_intern.h"

#ifndef DEBUG_SetFunction
#   define DEBUG_SetFunction 0
#endif
#undef DEBUG
#if DEBUG_SetFunction
#   define DEBUG 1
#endif
#include <aros/debug.h>
#undef kprintf

/* moveb %d0, %a3@+
 * rts
 */
const ULONG m68k_string = 0x16c04e75;
/* addql #1, %a3@
 * rts
 */
const ULONG m68k_count  = 0x52934e75;
/* jmp %a6@(-86 * 6)
 */
const ULONG m68k_serial = 0x4eeefdfc;

static AROS_UFH5(APTR, myRawDoFmt,
	AROS_UFHA(CONST_STRPTR, fmt, A0),
	AROS_UFHA(APTR,        args, A1),
	AROS_UFHA(VOID_FUNC,  putch, A2),
	AROS_UFHA(APTR,      putptr, A3),
	AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    switch ((IPTR)putch) {
    case (IPTR)RAWFMTFUNC_STRING:
	putch = (VOID_FUNC)&m68k_string;
	break;
    case (IPTR)RAWFMTFUNC_COUNT:
	putch = (VOID_FUNC)&m68k_count;
	break;
    case (IPTR)RAWFMTFUNC_SERIAL:
	putch = (VOID_FUNC)&m68k_serial;
	break;
    default:
	break;
    }

    return AROS_UFC5(APTR, ((struct IntExecBase *)SysBase)->PlatformData.realRawDoFmt,
	AROS_UFCA(CONST_STRPTR, fmt, A0),
	AROS_UFCA(APTR,        args, A1),
	AROS_UFCA(VOID_FUNC,  putch, A2),
	AROS_UFCA(APTR,      putptr, A3),
	AROS_UFCA(struct ExecBase *, SysBase, A6));

    AROS_USERFUNC_EXIT
}

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
	This contains a hack to fix dos.library/ramlib attempts to
	setfunction exec functions. Because of this, a funcOffset
	of more than 32 kB be truncated. This hack will also fix
	other programs only using the lower 16 bits of funcOffset
	and leaving garbage in the upper 16 bits.

	These programs should be fixed.

	Also, this includes a hack to fix attempt to SetFunction the
	Exec/RawDoFmt() routine, which adds a wrapper to translate
	'magic' AROS PutChFunc vectors to real functions.

    SEE ALSO
	MakeLibrary(), MakeFunctions(), SumLibrary()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    APTR ret;
    ULONG *vecaddr;

    D(bug("SetFunction(%s, -%lx, %lx) = ", (ULONG)library->lib_Node.ln_Name, -funcOffset, (ULONG)newFunction));

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

    /* AOS 3.1's locale.library wants to trample over Exec/RawDoFmt.
     * Since the replacement does not understand the 'magic' AROS
     * RAWFMTFUNC_xxxx vectors, we need to install a wrapper to
     * translate those to real functions.
     *
     * This should not effect AROS usage of SetFunction() of 
     * RawDoFmt, as it will only end up adding about 16 m68k
     * instructions of overhead.
     */
    if (library == (APTR)SysBase && funcOffset == (-87 * LIB_VECTSIZE)) {
    	((struct IntExecBase *)SysBase)->PlatformData.realRawDoFmt = newFunction;
    	newFunction = myRawDoFmt;
    }

    /*
	Arbitrate for the jumptable. This isn't enough for interrupt callable
	functions - but it need not be.
    */
    Forbid();

    /* Mark the library as changed. */
    library->lib_Flags|=LIBF_CHANGED;

    /* The following section is coded like this (instead of using the macros),
       because else gcc will output 64-bit muls instructions, that are not
       present on the 68060 (and will crash it). It's faster this way, too. :) */
    vecaddr = (APTR)((ULONG)library + funcOffset);

    /* Get the old vector pointer */
    ret = (APTR)*(ULONG *)(((ULONG)vecaddr)+2);

    /* Set new vector and jmp instruction */
    *(UWORD *)vecaddr = 0x4ef9;
    *(ULONG *)(((ULONG)vecaddr)+2) = (ULONG)newFunction;

#if 1
    CacheClearU();
#else
    /* Can't use this because there are broken programs that
     * copy code and then call SetFunction().
     */
    CacheClearE(vecaddr,LIB_VECTSIZE,CACRF_ClearI|CACRF_ClearD);
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

