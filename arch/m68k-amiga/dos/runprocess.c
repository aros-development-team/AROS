/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: RunProcess() - Run a process from an entry point with args
    Lang: english
*/
#include <aros/asmcall.h>	/* LONG_FUNC */
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include <string.h>

/* Ah, Dos/RunProcess. What fun.
 *
 * We need, to support older AmigaOS programs, the following register
 * setup:
 *
 * D0 - Length of arg string
 * A0 - Pointer to arg string
 * A1 - BCPL 'reverse' stack pointer - SP-1500
 * A2 - Global vector
 * A5 - BCPL 'jsr' routine
 * A6 - BCPL 'rts' routine
 */
static ULONG CallEntry(APTR pReturn_Addr, struct StackSwapStruct* sss,
		       STRPTR argptr, ULONG argsize, LONG_FUNC entry)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    ULONG ret;
    ULONG _n1 = (ULONG)(argptr);
    ULONG _n2 = (ULONG)(argsize);
    ULONG _n3 = (ULONG)(SysBase);
    APTR pr_GlobVec = me->pr_GlobVec;
    APTR tc_SPLower = me->pr_Task.tc_SPLower;

    __asm__ __volatile__(
	"move.l %%sp,%%a0\n\t"
	"movem.l %%d2-%%d7/%%a2-%%a6,%%a0@-\n\t"
	"move.l %5,%%a0@-\n\t"          /* sp+ 16 = SysBase */
	"move.l %4,%%a0@-\n\t"          /* sp+ 12 = argsize */
	"move.l %3,%%a0@-\n\t"          /* sp+ 8 = argptr */
	"move.l #0f,%%a0@-\n\t"         /* sp+ 4 = return address */
	"move.l %%a0@,%1\n\t"           /* Save return address */
	"move.l %2,%%a0@-\n\t"          /* sp+ 0 = address to go to */
	"move.l %6,%%a2\n\t"            /* A2 - Global Vector */
	"move.l %7,%%a1\n\t"            /* A1 - BCPL frame */
	"move.l %%a0,%%sp\n\t"
	"move.l %%sp@(8),%%a0\n\t"
	"move.l %%sp@(12),%%d0\n\t"
	"lea.l  BCPL_jsr,%%a5\n\t"      /* A5 - BCPL jsr routine */
	"lea.l  BCPL_rts,%%a6\n\t"      /* A6 - BCPL rts routine */
	"rts    \n\t"
	"0:\n\t"
	"lea.l   %%sp@(12),%%sp\n\t"
	"movem.l %%sp@+,%%d2-%%d7/%%a2-%%a6\n\t"
	"move.l  %%d0,%0"
	: "=g" (ret), "=m"(*(APTR *)pReturn_Addr)
	: "m" (entry), "m"(_n1), "m"(_n2), "m"(_n3), "m"(pr_GlobVec), "m"(tc_SPLower)
	: "cc", "memory", "%d0", "%d1", "%a0", "%a1" );

    return ret;
}

/**************************************************************************

    NAME */
	LONG AROS_SLIB_ENTRY(RunProcess,Dos) (

/*  SYNOPSIS */
	struct Process		* proc,
	struct StackSwapStruct  * sss,
	CONST_STRPTR		  argptr,
	ULONG			  argsize,
	LONG_FUNC		  entry,
	struct DosLibrary 	* DOSBase)

/* FUNCTION
	Sets the stack as specified and calls the routine with the given
	arguments.

    INPUTS
	proc	- Process context
	sss	- New Stack
	argptr	- Pointer to argument string
	argsize	- Size of the argument string
	entry	- The entry point of the function
	is_bcpl - Use BCPL entry
	DOSBase	- Pointer to dos.library structure

    RESULT
	The return value of (*entry)();

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

**************************************************************************/
{
    LONG ret;
    APTR oldReturnAddr = proc->pr_ReturnAddr; /* might be changed by CallEntry */
    struct StackSwapArgs args = {{
	(IPTR) &proc->pr_ReturnAddr,
	(IPTR) sss,
	(IPTR) argptr,
	argsize == -1 ? strlen(argptr) : argsize, /* Compute argsize automatically */
	(IPTR) entry,
	(IPTR) proc->pr_GlobVec,
    }};

    /* Call the function with the new stack */
    ret = NewStackSwap(sss, CallEntry, &args);

    proc->pr_ReturnAddr = oldReturnAddr;
    return ret;
}
