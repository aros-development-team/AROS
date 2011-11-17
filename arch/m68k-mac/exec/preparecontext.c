/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/ptrace.h>

#include "exec_util.h"

AROS_LH4(BOOL, PrepareContext,
         AROS_LHA(struct Task *, task, A0),
         AROS_LHA(APTR, entryPoint,   A1),
         AROS_LHA(APTR, fallBack,     A2),
         AROS_LHA(struct TagItem *, tagList, A3),
         struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    struct pt_regs *regs;
    
    UBYTE *sp = (UBYTE *)task->tc_SPReg;
    
    /* Push fallBack address */
    sp -= sizeof(APTR);
    *(APTR *)sp = fallBack;
    
    if (!(task->tc_Flags & TF_ETASK))
        return FALSE;

    task->tc_UnionETask.tc_ETask->et_RegFrame = AllocTaskMem(task, SIZEOF_ALL_REGISTERS,
                                                             MEMF_PUBLIC|MEMF_CLEAR);
    
    if (!(regs  = (struct pt_regs *)task->tc_UnionETask.tc_ETask->et_RegFrame))
        return FALSE;
    
    regs->usp = sp;
    regs->pc  = (ULONG)entryPoint;
    
    task->tc_SPReg = sp;
    
    return TRUE;
    AROS_LIBFUNC_EXIT
}
