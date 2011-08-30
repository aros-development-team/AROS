/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <aros/asmcall.h>

#include "dos_intern.h"

AROS_UFP2(ULONG, BCPL_Exit,
    AROS_UFPA(LONG, returnCode, D1),
    AROS_UFPA(struct DosLibrary *, DOSBase, A6));

AROS_LH1(void, Exit,
	 AROS_LHA(LONG, returnCode, D1),
	 struct DosLibrary *, DOSBase, 24, Dos)
{
    AROS_LIBFUNC_INIT

    AROS_UFC2(ULONG, BCPL_Exit,
	AROS_UFCA(LONG, returnCode, D1),
	AROS_UFCA(struct DosLibrary *, DOSBase, A6));

    AROS_LIBFUNC_EXIT
}

extern ULONG AOS_CallEntry(void);

/*
 * This entry code is used by CreateNewProc(). It supposes that it calls
 * normal C code and follows AROS ABI conventions by putting SysBase into A6.
 * Note that it still needs to set pr_ReturnAddr.
 * pr_ReturnAddr value is used by AROS libc as program identifier (it is
 * supposed to contain a unique value per program).
 */
ULONG CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me)
{
    return AROS_UFC6(ULONG, AOS_CallEntry,
            AROS_UFCA(ULONG,  argsize, D0),
            AROS_UFCA(STRPTR, argptr, A0),
            AROS_UFCA(APTR, me->pr_Task.tc_SPLower, A1),
            AROS_UFCA(APTR, me->pr_GlobVec, A2),
            AROS_UFCA(APTR, &me->pr_ReturnAddr, A3),
            AROS_UFCA(LONG_FUNC, entry, A4));
}
