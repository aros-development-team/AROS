/*
    Copyright © 2015-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/task.h>

#include "etask.h"

#include "task_intern.h"

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
        switch(Tag->ti_Tag)
        {
        case(TaskTag_CPUNumber):
#if defined(__AROSEXEC_SMP__)
            *((IPTR *)Tag->ti_Data) = task_et->iet_CpuNumber;
#else
            *((IPTR *)Tag->ti_Data) = 0;
#endif
            break;
        case(TaskTag_CPUAffinity):
#if defined(__AROSEXEC_SMP__)
            {
                int i, count = KrnGetCPUCount();
                for (i = 0; i < count; i ++)
                {
                    if (KrnCPUInMask(i, task_et->iet_CpuAffinity))
                        KrnGetCPUMask(i, (void *)Tag->ti_Data);
                }
            }
#endif
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
        }
    }

    AROS_LIBFUNC_EXIT
} /* QueryTaskTagList() */
