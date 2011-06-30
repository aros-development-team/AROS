/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

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

AROS_UFP7(ULONG, DOS_CallEntry_A6,
	AROS_UFPA(STRPTR, argptr, A0),
	AROS_UFPA(ULONG,  argsize, D0),
	AROS_UFPA(LONG_FUNC, entry, A3),
	AROS_UFPA(APTR, globvec, A2),
	AROS_UFPA(APTR, splower, A1),
	AROS_UFPA(APTR, returnaddr, A4),
	AROS_UFPA(APTR, a6, A6));

/*
 * Call BCPL-compatible entry point. BCPL ABI requires the following:
 * D0 - Length of arg string
 * A0 - Pointer to arg string
 * A1 - BCPL 'reverse' stack pointer - tc_SPLower
 * A2 - Global vector
 * A5 - BCPL 'jsr' routine
 * A6 - BCPL 'rts' routine
 *
 * Note that in this version, 'A6' is passed in, so that we can
 * support both the BCPL style (A6 is SysBase) and CreateProcess
 * style (A6 is BCPL_rts)
 */
static inline ULONG CallEntry_A6(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me, APTR a6)
{
    return AROS_UFC7(ULONG, DOS_CallEntry_A6,
	    AROS_UFCA(STRPTR, argptr, A0),
	    AROS_UFCA(ULONG,  argsize, D0),
	    AROS_UFCA(LONG_FUNC, entry, A3),
	    AROS_UFCA(APTR, me->pr_GlobVec, A2),
	    AROS_UFCA(APTR, me->pr_Task.tc_SPLower, A1),
	    AROS_UFCA(APTR, &me->pr_ReturnAddr, A4),
	    AROS_UFCA(APTR, a6, A6));
}

/*
 * This entry code is used by CreateNewProc(). It supposes that it calls
 * normal C code and follows AROS ABI conventions by putting SysBase into A6.
 * Note that it still needs to set pr_ReturnAddr.
 * pr_ReturnAddr value is used by AROS libc as program identifier (it is
 * supposed to contain a unique value per program).
 */
ULONG CallEntry(STRPTR argPtr, ULONG argSize, LONG_FUNC entry, struct Process *me)
{
    return CallEntry_A6(argPtr, argSize, entry, me, SysBase);
}

/*
 * Call BCPL-compatible entry point. BCPL ABI requires the following:
 * D0 - Length of arg string
 * A0 - Pointer to arg string
 * A1 - BCPL 'reverse' stack pointer - tc_SPLower
 * A2 - Global vector
 * A5 - BCPL 'jsr' routine
 * A6 - BCPL 'rts' routine
 */
ULONG BCPL_CallEntry(STRPTR argPtr, ULONG argSize, LONG_FUNC entry, struct Process *me)
{
    extern void *BCPL_rts;

    return CallEntry_A6(argPtr, argSize, entry, me, &BCPL_rts);
}
