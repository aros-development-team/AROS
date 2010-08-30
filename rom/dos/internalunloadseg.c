/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, InternalUnLoadSeg,

/*  SYNOPSIS */
        AROS_LHA(BPTR     , seglist , D1),
        AROS_LHA(VOID_FUNC, freefunc, A1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 127, Dos)

/*  FUNCTION
	Unloads a seglist loaded with InternalLoadSeg().

    INPUTS
	seglist  - Seglist
	freefunc - Function to be called to free memory

    RESULT
	DOSTRUE if everything wents O.K.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR next;

    if (seglist)
    {
#ifdef KrnUnregisterModule
	if (KernelBase)
	    KrnUnregisterModule(seglist);
#endif

	while (seglist) {
	    char *seg = (ULONG)seglist;

	    next = *(BPTR *)BADDR(seglist);
	    seg += (*(LONG *)((LONG)BADDR(seglist) - sizeof(ULONG))) / 2;

	    AROS_CALL2NR(void, freefunc,
			 AROS_LCA(APTR ,  (BPTR *)((LONG)BADDR(seglist) - sizeof(ULONG)), A1),
			 AROS_LCA(ULONG, *(LONG *)((LONG)BADDR(seglist) - sizeof(ULONG)), D0),
			 struct Library *, (struct Library *)SysBase);

	    seglist = next;
	}

	return DOSTRUE;
    }
    else
	return DOSFALSE;

    AROS_LIBFUNC_EXIT
} /* InternalUnLoadSeg */
