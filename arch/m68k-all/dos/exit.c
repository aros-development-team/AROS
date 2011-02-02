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

/*
 * We need, to support older AmigaOS programs, the following register
 * setup:
 *
 * D0 - Length of arg string
 * A0 - Pointer to arg string
 * A1 - BCPL 'reverse' stack pointer - tc_SPLower
 * A2 - Global vector
 * A5 - BCPL 'jsr' routine
 * A6 - BCPL 'rts' routine
 */
ULONG CallEntry(STRPTR argptr, ULONG argsize, LONG_FUNC entry, struct Process *me)
{
    ULONG ret;
    APTR pr_GlobVec = me->pr_GlobVec;
    APTR tc_SPLower = me->pr_Task.tc_SPLower;

    __asm__ __volatile__(
	"move.l %%sp,%%a0\n\t"
	"movem.l %%d2-%%d7/%%a2-%%a6,%%a0@-\n\t"
	"move.l %4,%%d0\n\t"            /* stksize = %a0 - tc_SPLower */
	"neg.l  %%d0\n\t"
	"lea.l  %%a0@(%%d0),%%a1\n\t"
	"move.l %%a1,%%a0@-\n\t"        /* sp+ 8 = stksize  */
	"lea.l  %%a0@,%%a2\n\t"         /* Save address of return address */
	"move.l %%a2,%1\n\t"
	"move.l #0f,%%a0@-\n\t"         /* sp+ 4 = return address */
	"move.l %2,%%a0@-\n\t"          /* sp+ 0 = address to go to */
	"move.l %%a0,%%d1\n\t"
	"move.l %3,%%a2\n\t"            /* A2 - Global Vector */
	"move.l %4,%%a1\n\t"            /* A1 - BCPL frame */
	"move.l %5,%%a0\n\t"            /* A0 - Argptr */
	"move.l %6,%%d0\n\t"            /* D0 - Argsize */
	"move.l %%d1,%%sp\n\t"
	"lea.l  BCPL_jsr,%%a5\n\t"      /* A5 - BCPL jsr routine */
	"lea.l  BCPL_rts,%%a6\n\t"      /* A6 - BCPL rts routine */
	"rts    \n\t"
	"0:\n\t"
	"addq.l  #4,%%sp\n\t"
	"movem.l %%sp@+,%%d2-%%d7/%%a2-%%a6\n\t"
	"move.l  %%d0,%0"
	: "=g" (ret), "=m"(me->pr_ReturnAddr)
	: "m" (entry), "m"(pr_GlobVec), "m"(tc_SPLower),
	  "m" (argptr), "m" (argsize)
	: "cc", "memory", "%d0", "%d1", "%a0", "%a1", "%a2", "%a3" );

    return ret;
}
