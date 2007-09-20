#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <sigcore.h>
#include <asm/cpu.h>
#include "etask.h"
#include "exec_util.h"

#define DEBUG 1

#include <aros/libcall.h>
#include <aros/debug.h>
#include <asm/segments.h>

#define Regs(t) ((struct regs_t *)(GetIntETask(t)->iet_Context))

static UQUAD *PrepareContext_Common(struct Task *task, APTR entryPoint, APTR fallBack,
                                    struct TagItem *tagList, struct ExecBase *SysBase)
{
    regs_t      *regs;
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
    
    if (!(regs = (ULONG*)GetIntETask (task)->iet_Context))
        return NULL;

    if (numargs)
    {
        switch (numargs)
        {
            case 8:
                *--sp = args[7];
            case 7:
                *--sp = args[6];
            case 6:
                regs->r9 = args[5];
            case 5:
                regs->r8 = args[4];
            case 4:
                regs->rcx = args[3];
            case 3:
                regs->rdx = args[2];
            case 2:
                regs->rsi = args[1];
            case 1:
                regs->rdi = args[0];
                break;
        }
    }

    /* Push fallBack address */
    *--sp = fallBack;

    regs->ds = USER_DS;
    regs->return_rip = (IPTR)entryPoint;
    regs->return_cs = USER_CS;
    regs->return_rflags = 0x3202;
    regs->return_ss = USER_DS;
    regs->return_rsp = (IPTR)sp;
    
    task->tc_SPReg = sp;
 
    UBYTE current_xmm[512+16], *curr = current_xmm;
    
    curr = (UBYTE*)(((IPTR)curr + 15) & ~15);
    IPTR sse_ctx = ((IPTR)regs + sizeof(regs_t) + 15) & ~15;
    
    asm volatile("fxsave (%0); fninit; fwait; fxsave (%1); fxrstor (%0);"::"r"(curr),"r"(sse_ctx));
    
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
