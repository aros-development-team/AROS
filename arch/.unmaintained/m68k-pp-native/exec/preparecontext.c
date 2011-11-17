/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/ptrace.h>

#include "exec_util.h"

BOOL PrepareContext(struct Task *task, APTR entryPoint, APTR fallBack,
                    struct TagItem *tagList, struct ExecBase *SysBase)
{
    struct pt_regs *regs;
    UBYTE *sp = (UBYTE *)task->tc_SPReg;
    
    /* Push fallBack address */
    sp -= sizeof(APTR);
    *(APTR *)sp = fallBack;
    
    if (!(task->tc_Flags & TF_ETASK))
    {
        *(ULONG *)0x0bad0001=0;
        return FALSE;
    }
      
    task->tc_UnionETask.tc_ETask->et_RegFrame = AllocTaskMem(task, SIZEOF_ALL_REGISTERS,
                                                             MEMF_PUBLIC|MEMF_CLEAR);
    
    if (!(regs  = (struct pt_regs *)task->tc_UnionETask.tc_ETask->et_RegFrame))
    {
        *(ULONG *)0x0bad0003=0;
        return FALSE;
    }
    
    regs->usp = sp;
    regs->pc  = (ULONG)entryPoint;
    
    task->tc_SPReg = sp;
    
    return TRUE;
}
