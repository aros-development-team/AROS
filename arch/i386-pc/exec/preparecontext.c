/*
    Copyright (C) 1995-1997 AROS - The Amiga Research OS
    $Id$

    Desc: i386unix version of PrepareContext().
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <sigcore.h>
#include "etask.h"
#include "exec_util.h"

#include <aros/libcall.h>
#include <asm/ptrace.h>
#include <asm/segments.h>

#define Regs(t) ((struct pt_regs *)(GetIntETask(t)->iet_Context))

AROS_LH3(BOOL, PrepareContext,
    AROS_LHA(struct Task *, task, A0),
    AROS_LHA(APTR, entryPoint, A1),
    AROS_LHA(APTR, fallBack, A2),
    struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    ULONG *regs;

    UBYTE *sp=(UBYTE *)task->tc_SPReg;

    /* Push fallBack address */

    sp-=sizeof(APTR);
    *(APTR*)sp=fallBack;
    
    if (!(task->tc_Flags & TF_ETASK) )
        return FALSE;

    GetIntETask (task)->iet_Context = AllocTaskMem (task
        , SIZEOF_ALL_REGISTERS
        , MEMF_PUBLIC|MEMF_CLEAR
    );

    if (!(regs=(ULONG*)GetIntETask (task)->iet_Context))
        return FALSE;

    /* We have to prepare whole context right now so Dispatch()
     * would work propertly */

    *regs++ = 0;		/* ebx */
    *regs++ = 0;		/* ecx */
    *regs++ = 0;		/* edx */
    *regs++ = 0;		/* esi */
    *regs++ = 0;		/* edi */
    *regs++ = 0;		/* ebp */
    *regs++ = 0;		/* eax */
    *regs++ = USER_DS;		/* xds */
    *regs++ = USER_DS;		/* xes */
    *regs++ = entryPoint;	/* eip */
    *regs++ = USER_CS;		/* xcs */
    *regs++ = 0x3202;		/* eflags */
    *regs++ = sp;		/* esp */
    *regs++ = USER_DS;		/* xss */

    task->tc_SPReg = sp;
    
    /* We return the new stack pointer back to the caller. */
    return TRUE;

    AROS_LIBFUNC_EXIT
}
