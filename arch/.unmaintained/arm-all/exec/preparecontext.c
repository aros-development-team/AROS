/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/ptrace.h>
#include "etask.h"
#include "exec_util.h"

#define DEBUG 1
#include <aros/debug.h>

#error "PrepareContext() has been changed. Additional tagList param, etc."
#error "This one here needs to be rewritten!"

AROS_LH3(BOOL, PrepareContext,
         AROS_LHA(struct Task *, task, A0),
         AROS_LHA(APTR, entryPoint,   A1),
         AROS_LHA(APTR, fallBack,     A2),
         struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    struct pt_regs *regs;

    ULONG *sp = (ULONG *)task->tc_SPReg;
    ULONG * armregs;
    ULONG i = 0;

    if (!(task->tc_Flags & TF_ETASK)) {
        return FALSE;
    }

    task->tc_UnionETask.tc_ETask->et_RegFrame = AllocTaskMem (task, SIZEOF_ALL_REGISTERS,
                                                              MEMF_PUBLIC|MEMF_CLEAR);

    if (!(regs  = (struct pt_regs *)task->tc_UnionETask.tc_ETask->et_RegFrame))
        return FALSE;

    /*
     * Fill the registers r0-r3 with the arguments on the stack -
     * if there are any...
     */
    armregs = &regs->r0;
#if 0
    D(bug("sp=%x, task->tc_SPUpper=%x, name=%s\n",
          sp,
          task->tc_SPUpper,
          task->tc_Node.ln_Name));
#endif
    if (sp < (ULONG *)task->tc_SPUpper) {
        while (sp < (ULONG *)task->tc_SPUpper && i < 4) {
//          D(bug("before: armregs=%x, sp=%x\n",armregs,sp));
            *armregs = *sp;
            armregs++;
            sp++;
            i++;
//          D(bug("after: armregs=%x, sp=%x\n",armregs,sp));
        }
        sp--;
    }
    
    /*
     * Initialize initial registers. Only sp and lr_svc
     * are important. lr_svc represents the entry point of
     * the process plus 4. The 4 will be subtracted automatically
     * when the process is started. It must be done that way
     * due to how the ARM processor works when handling *interrupts*!
     * Also set the processor for User Mode.
     */
    regs->sp      = sp;
    regs->lr_svc  = (ULONG)entryPoint+4;
    regs->cpsr    = (ULONG)0x10;

    /*
     * Now I am not sure whether this is going to work. 
     * In the *startup* code I will need to make sure
     * that the fallback can be executed!
     * Note that no '4' needs to be added in this case!
     */
    regs->lr      = (ULONG)fallBack;

    return TRUE;
    AROS_LIBFUNC_EXIT
}
