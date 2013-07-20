/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

/* See rom/kernel/copymem.c for documentation */

AROS_LH3I(void, CopyMem,
    AROS_LHA(CONST_APTR,  source, A0),
    AROS_LHA(APTR,  dest,   A1),
    AROS_LHA(IPTR,  size,   D0),
    struct ExecBase *, SysBase, 104, Exec)
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
