/*
    Copyright (C) 2015-2025, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/task.h>

#include "etask.h"

#include "task_intern.h"

#if defined(__mc68000__)
#include <aros/m68k/cpucontext.h>
#elif defined(__x86_64__)
#include <aros/x86_64/cpucontext.h>
#elif defined(__i386__)
#include <aros/i386/cpucontext.h>
#endif

/*****************************************************************************

    NAME */
#include <proto/task.h>

        AROS_LH2(void, QueryTaskTagList,

/*  SYNOPSIS */
        AROS_LHA(struct Task *, task, A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct TaskResBase *, TaskResBase, 6, Task)

/*  FUNCTION

        Provides information about selected system Task
    
    INPUTS

        Function takes an array of tags. Data is returned for each tag. See
        specific tag description.

    TAGS

        TaskTag_CPUNumber - (IPTR *) Returns the CPU Number the task is currently running on
        TaskTag_CPUAffinity - (IPTR *) Returns the CPU Affinity mask
        TaskTag_CPUTime - (struct timeval *) Returns the amount of cpu time a task has used .
        TaskTag_StartTime - (struct timeval *) Returns the time the task was launched .

    RESULT

        None

    NOTES
    
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem * Tag = NULL;
    struct Library *UtilityBase = TaskResBase->trb_UtilityBase;
    struct IntETask *task_et = GetIntETask(task);

    D(
        bug("[TaskRes] %s: task @ 0x%p\n", __func__, task);
        bug("[TaskRes] %s: taglist @ 0x%p\n", __func__, tagList);
    )

    /* This is the default implementation */
        
    while ((Tag = NextTagItem(&tagList)) != NULL)
    {
        if (Tag->ti_Tag > TaskTag_REG_General0 && Tag->ti_Tag < TaskTag_REG_FloatCount)
        {
            // General Purpose Registers...
                IPTR *storeval = (IPTR *)Tag->ti_Data;
                if (storeval && GetETask(task))
                {
#if defined(__x86_64__)
                    struct ExceptionContext *regs = (struct ExceptionContext *)GetETask(task)->et_RegFrame;
                    IPTR *regptr = (IPTR *)((IPTR)&regs->rax + ((Tag->ti_Tag - TaskTag_REG_General0) * sizeof(UQUAD)));
                    *storeval = *regptr;
#endif
                }
                continue;
        }
        else if (Tag->ti_Tag > TaskTag_REG_Float0 && Tag->ti_Tag < TaskTag_REG_VecCount)
        {
            // Floating Point Registers...
                IPTR *storeval = (IPTR *)Tag->ti_Data;
                if (storeval && GetETask(task))
                {
#if defined(__x86_64__)
                    MMReg *regstr = (MMReg *)storeval;
                    struct ExceptionContext *regs = (struct ExceptionContext *)GetETask(task)->et_RegFrame;
                    MMReg *regptr = (MMReg *)((IPTR)&regs->FXSData->mm[0] + ((Tag->ti_Tag - TaskTag_REG_Float0) * sizeof(MMReg)));
                    *regstr = *regptr;
#endif
                }
                continue;
        }
        else if (Tag->ti_Tag > TaskTag_REG_Vec0 && Tag->ti_Tag < (TaskTag_REG_Vec0 + 0x18))
        {
            // Vector Registers...
                IPTR *storeval = (IPTR *)Tag->ti_Data;
                if (storeval && GetETask(task))
                {
#if defined(__x86_64__)
                    struct ExceptionContext *regs = (struct ExceptionContext *)GetETask(task)->et_RegFrame;
                    IPTR *regptr = (IPTR *)((IPTR)&regs->rax + ((Tag->ti_Tag - TaskTag_REG_General0) * sizeof(UQUAD)));
                    *storeval = *regptr;
#endif
                }
                continue;
        }

        switch(Tag->ti_Tag)
        {
        case(TaskTag_CPUNumber):
            {
#if defined(__AROSEXEC_SMP__)
            *((IPTR *)Tag->ti_Data) = task_et->iet_CpuNumber;
#else
            *((IPTR *)Tag->ti_Data) = 0;
#endif
            }
            break;
        case(TaskTag_CPUAffinity):
            {
#if defined(__AROSEXEC_SMP__)
                int i, count = KrnGetCPUCount();
                for (i = 0; i < count; i ++)
                {
                    if (KrnCPUInMask(i, task_et->iet_CpuAffinity))
                        KrnGetCPUMask(i, (void *)Tag->ti_Data);
                }
#endif
            }
            break;
        case(TaskTag_CPUTime):
            {
                struct timeval *storeval = (struct timeval *)Tag->ti_Data;
                if (task_et)
                {
                    storeval->tv_micro = (task_et->iet_CpuTime.tv_nsec + 500) / 1000;
                    storeval->tv_secs  = task_et->iet_CpuTime.tv_sec;
                }
            }
            break;
        case(TaskTag_CPUUsage):
            {
                if (task_et)
                    *(ULONG *)(Tag->ti_Data) = task_et->iet_CpuUsage;
                else
                    *(ULONG *)(Tag->ti_Data) = 0;
            }
            break;
        case(TaskTag_StartTime):
            {
                struct timeval *storeval = (struct timeval *)Tag->ti_Data;
                if (task_et)
                {
                    storeval->tv_micro = (task_et->iet_StartTime.tv_nsec + 500) / 1000;
                    storeval->tv_secs  = task_et->iet_StartTime.tv_sec;
                }
            }
            break;
        case(TaskTag_REG_GeneralCount):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
#if defined(__mc68000__)
                *storeval = 16; // d0-d8/a0-a8
#elif defined(__i386__) || defined(__x86_64__)
                *storeval = 15; // RAX through R15
#endif
            }
            break;
        case(TaskTag_REG_GeneralSize):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
#if defined(__i386__) || defined(__x86_64__)
                *storeval = sizeof(IPTR);
#endif
            }
            break;
        case(TaskTag_REG_FloatCount):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
#if defined(__mc68000__)
                *storeval = 8;
#elif defined(__i386__) || defined(__x86_64__)
                *storeval = 8; // ST0 through ST7
#endif
            }
            break;
        case(TaskTag_REG_FloatSize):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
#if defined(__mc68000__)
                *storeval = 12; // 96bit
#elif defined(__i386__) || defined(__x86_64__)
                *storeval = 10; // 80bit
#endif
            }
            break;
        case(TaskTag_REG_VecCount):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
#if defined(__mc68000__)
                *storeval = 0;
#elif defined(__i386__) || defined(__x86_64__)
                *storeval = 32; // X/Y/ZMM0 through X/Y/ZMM31
#endif
            }
            break;
        case(TaskTag_REG_VecSize):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
#if defined(__i386__) || defined(__x86_64__)
                // We only report SSE size just now.
                *storeval = 128/8;
                //*storeval = 256/8;
                //*storeval = 512/8;
#endif
            }
            break;
        case(TaskTag_REG_SP):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
                if (storeval)
                {
                    struct ExceptionContext *regs = (struct ExceptionContext *)GetETask(task)->et_RegFrame;
#if defined(__mc68000__) 
                    *storeval = (IPTR)regs->sr;
#elif defined(__i386__) 
                    *storeval = (IPTR)regs->esp;
#elif defined(__x86_64__)
                    *storeval = (IPTR)regs->rsp;
#endif
                }
            }
            break;
        case(TaskTag_REG_PC):
            {
                IPTR *storeval = (IPTR *)Tag->ti_Data;
                if (storeval && GetETask(task))
                {
                    struct ExceptionContext *regs = (struct ExceptionContext *)GetETask(task)->et_RegFrame;
#if defined(__mc68000__) 
                    *storeval = (IPTR)regs->pc;
#elif defined(__i386__) 
                    *storeval = (IPTR)regs->eip;
#elif defined(__x86_64__)
                    *storeval = (IPTR)regs->rip;
#endif
                }
            }
            break;
        }
    }

    AROS_LIBFUNC_EXIT
} /* QueryTaskTagList() */
