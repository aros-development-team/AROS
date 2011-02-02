/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include "dos_intern.h"

AROS_LH1(void, Exit,
	 AROS_LHA(LONG, returnCode, D1),
	 struct DosLibrary *, DOSBase, 24, Dos)
{
    AROS_LIBFUNC_INIT

    /* TODO: implement this according to CallEntry() implementation */

    AROS_LIBFUNC_EXIT
}

static ULONG CallEntry(ULONG CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me)
{
    /*
     * TODO:
     * 1. Rewrite the thing in clean asm, remove the obsolete macro from cpu.h.
     * 2. Fix up the function for better compatibility. See comments in rom/dos/exit.c.
     */
    return AROS_UFC3R(ULONG, entry,
		      AROS_UFCA(STRPTR, argptr, A0),
		      AROS_UFCA(ULONG, argsize, D0),
		      AROS_UFCA(struct ExecBase *, SysBase, A6),
		      &me->pr_ReturnAddr,
		      me->pr_Task.tc_SPUpper - me->pr_Task.tc_SPLower
		     );

}
