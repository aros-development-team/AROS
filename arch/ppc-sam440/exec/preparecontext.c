#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <asm/amcc440.h>
#include "etask.h"
#include "exec_util.h"

#define DEBUG 1

#include <aros/libcall.h>
#include <aros/debug.h>

#define Regs(t) ((struct regs_t *)(GetIntETask(t)->iet_Context))

static UQUAD *PrepareContext_Common(struct Task *task, APTR entryPoint, APTR fallBack,
                                    struct TagItem *tagList, struct ExecBase *SysBase)
{
    context_t   *ctx;
    int         i;
    IPTR        *sp=(IPTR *)task->tc_SPReg;
    IPTR        args[8] = {0};
    WORD        numargs = 0;
 
    while(tagList)
    {
        switch(tagList->ti_Tag)
        {
            case TAG_MORE:
                tagList = (struct TagItem *)tagList->ti_Data;
                continue;

            case TAG_SKIP:
                tagList += tagList->ti_Data;
                break;

            case TAG_DONE:
                tagList = NULL;
                break;
                
            #define HANDLEARG(x) \
            case TASKTAG_ARG ## x: \
                args[x - 1] = (IPTR)tagList->ti_Data; \
                if (x > numargs) numargs = x; \
                break;

            HANDLEARG(1)
            HANDLEARG(2)
            HANDLEARG(3)
            HANDLEARG(4)
            HANDLEARG(5)
            HANDLEARG(6)
            HANDLEARG(7)
            HANDLEARG(8)

            #undef HANDLEARG
            }

        if (tagList) tagList++;
    }

    if (!(task->tc_Flags & TF_ETASK) )
        return NULL;

    GetIntETask (task)->iet_Context = AllocTaskMem (task
        , SIZEOF_ALL_REGISTERS
        , MEMF_PUBLIC|MEMF_CLEAR
    );

    D(bug("[exec] PrepareContext: iet_Context = %012p\n", GetIntETask (task)->iet_Context));
    
    if (!(ctx = (context_t *)GetIntETask (task)->iet_Context))
        return NULL;

    if (numargs)
    {
        switch (numargs)
        {
            case 8:
                ctx->cpu.gpr[10] = args[7];
            case 7:
                ctx->cpu.gpr[9] = args[6];
            case 6:
                ctx->cpu.gpr[8] = args[5];
            case 5:
                ctx->cpu.gpr[7] = args[4];
            case 4:
                ctx->cpu.gpr[6] = args[3];
            case 3:
                ctx->cpu.gpr[5] = args[2];
            case 2:
                ctx->cpu.gpr[4] = args[1];
            case 1:
                ctx->cpu.gpr[3] = args[0];
                break;
        }
    }

    /* Push fallBack address */
    ctx->cpu.lr = fallBack;
    /* 
     * Task will be started upon interrupt resume. Push entrypoint into SRR0 
     * and the MSR register into SRR1. Enable FPU at the beginning
     */
    ctx->cpu.srr0 = (IPTR)entryPoint;
    ctx->cpu.srr1 = MSR_PR | MSR_EE | MSR_CE | MSR_ME;
    ctx->cpu.srr1 |= MSR_FP;
    ctx->cpu.gpr[1] = sp;
    
    task->tc_SPReg = sp;

    for (i=0; i < 32; i++)
        ctx->fpu.fpr[i] = 0.0;
    
    ctx->fpu.fpscr = 0;
        
    return sp;
}



AROS_LH4(BOOL, PrepareContext,
    AROS_LHA(struct Task *, task, A0),
    AROS_LHA(APTR, entryPoint, A1),
    AROS_LHA(APTR, fallBack, A2),
    AROS_LHA(struct TagItem *, tagList, A3),
    struct ExecBase *, SysBase, 6, Exec)
{
    AROS_LIBFUNC_INIT

    return PrepareContext_Common(task, entryPoint, fallBack, tagList, SysBase) ? TRUE : FALSE;

    AROS_LIBFUNC_EXIT
}
