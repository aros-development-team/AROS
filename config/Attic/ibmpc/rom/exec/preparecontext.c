/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: ibmpc version of PrepareContext().
    Lang: english
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include "etask.h"
#include "exec_util.h"
#include <asm/segments.h>

#include <aros/libcall.h>

void StoreFPU(APTR);

AROS_LH3(BOOL, PrepareContext,
    AROS_LHA(struct Task *, task, A0),
    AROS_LHA(APTR, entryPoint, A1),
    AROS_LHA(APTR, fallBack, A2),
    struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    UBYTE *sp=(UBYTE *)task->tc_SPReg;
    
    /* Push fallBack address */

    sp-=sizeof(APTR);
    *(APTR*)sp=fallBack;
    
    /* Now emulate entered supervisor mode. Push eflags register */
    
    sp-=sizeof(ULONG);
    *(ULONG*)sp=0x202;	/* Ints enabled */
    
    /* Push CS */
    
    sp-=sizeof(ULONG);
    *(ULONG*)sp=KERNEL_CS;
    
    /* Push the context */
    
    sp-=sizeof(APTR);
    *(APTR*)sp=entryPoint;
    
    /* In this version there is no need to store registers on stack */
    
    if (!(task->tc_Flags & TF_ETASK) )
	return FALSE;

    GetIntETask (task)->iet_Context = AllocTaskMem (task
	, SIZEOF_ALL_REGISTERS
	, MEMF_PUBLIC|MEMF_CLEAR
    );

    if (!GetIntETask (task)->iet_Context)
	return FALSE;

    GetIntETask (task)->iet_FPU = AllocTaskMem (task
	, SIZEOF_FPU_STATE
	, MEMF_PUBLIC|MEMF_CLEAR
    );

    if (!GetIntETask (task)->iet_FPU)
	return FALSE;

    /* Create initial FPU state */
    asm ("finit");
    StoreFPU(GetIntETask (task)->iet_FPU);

    /* Store new sp */
    
    task->tc_SPReg=sp;

    return TRUE;

    AROS_LIBFUNC_EXIT
}
