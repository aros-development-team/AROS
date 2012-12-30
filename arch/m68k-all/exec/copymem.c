/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: CopyMem()
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <aros/libcall.h>

extern void AROS_SLIB_ENTRY(CopyMem_000,Exec,104)(void);
extern void AROS_SLIB_ENTRY(CopyMem_020,Exec,104)(void);

/*****************************************************************************

    NAME */

	AROS_LH3I(void, CopyMem,

/*  SYNOPSIS */
	AROS_LHA(CONST_APTR,  source, A0),
	AROS_LHA(APTR,  dest,   A1),
	AROS_LHA(IPTR,  size,   D0),

/*  LOCATION */
	struct ExecBase *, SysBase, 104, Exec)

/*  FUNCTION
	Copy some memory from one destination in memory to another using
	a fast copying method.

    INPUTS
	source - Pointer to source area
	dest   - Pointer to destination
	size   - number of bytes to copy (may be zero)

    RESULT

    NOTES
	The source and destination area are not allowed to overlap.

    EXAMPLE

    BUGS

    SEE ALSO
	CopyMemQuick()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT
    void (*func)(void);

    Disable();
    if (SysBase->AttnFlags & AFF_68020) {
        /* 68020+ */
        func = AROS_SLIB_ENTRY(CopyMem_020, Exec, 104);
    } else {
        /* 68000/68010 */
        func = AROS_SLIB_ENTRY(CopyMem_000, Exec, 104);
    }
    SetFunction((struct Library *)SysBase, -LIB_VECTSIZE * 104, func);
    Enable();

    return CopyMem(source, dest, size);

    AROS_LIBFUNC_EXIT
} /* CopyMem */
