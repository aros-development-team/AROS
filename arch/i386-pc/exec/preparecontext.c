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

    UBYTE *sp=(UBYTE *)task->tc_SPReg;

//	kprintf("Prepare context\n");
    
    /* Push fallBack address */

    sp-=sizeof(APTR);
    *(APTR*)sp=fallBack;
    
    if (!(task->tc_Flags & TF_ETASK) )
        return FALSE;

//	kprintf("ETask...\n");

    GetIntETask (task)->iet_Context = AllocTaskMem (task
        , SIZEOF_ALL_REGISTERS
        , MEMF_PUBLIC|MEMF_CLEAR
    );

//	kprintf("Allocated %d bytes for context at %p\n", SIZEOF_ALL_REGISTERS,
//		GetIntETask(task)->iet_Context);

    if (!GetIntETask (task)->iet_Context)
        return FALSE;

    /* We have to prepare whole context right now so Dispatch()
     * would work propertly */
	
    Regs(task)->xds     = USER_DS;
    Regs(task)->xes     = USER_DS;
    Regs(task)->xss     = USER_DS;
    Regs(task)->xcs     = USER_CS;
    Regs(task)->eip     = entryPoint;
    Regs(task)->esp     = sp;
    Regs(task)->eflags  = 0x3202;
    
//	kprintf("Registers set up\n");
	
    task->tc_SPReg = sp;
    
    /* We return the new stack pointer back to the caller. */
    return TRUE;

    AROS_LIBFUNC_EXIT
}
