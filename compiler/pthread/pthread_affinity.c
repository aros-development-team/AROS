/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Conversion between cpu_set_t and kernel CPU masks, for the
          pthread affinity functions.
*/

#include <aros/config.h>
#include <proto/exec.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/task.h>
#include <resources/task.h>

#include <errno.h>

#include "pthread_intern.h"
#include "debug.h"

/*
 * Check that set names at least one CPU that exists. CPUs beyond the
 * system's are ignored, as glibc does. On SMP builds *maskptr receives a
 * KrnAllocCPUMask() mask of those CPUs (the caller frees or hands it on);
 * on uniprocessor builds only CPU 0 exists and no mask is made.
 */
int _pthread_cpuset_to_mask(size_t setsize, const cpu_set_t *set, APTR *maskptr)
{
    APTR KernelBase = OpenResource("kernel.resource");
    unsigned int i, count, found = 0;
#if defined(__AROSEXEC_SMP__)
    APTR mask = NULL;
#endif

    *maskptr = NULL;
    if (!set)
        return EINVAL;

    count = KernelBase ? KrnGetCPUCount() : 1;

#if defined(__AROSEXEC_SMP__)
    if (KernelBase)
    {
        if (!(mask = KrnAllocCPUMask()))
            return ENOMEM;
        KrnClearCPUMask(mask);
    }
#endif

    for (i = 0; i < count; i++)
    {
        if (CPU_ISSET_S(i, setsize, set))
        {
            found++;
#if defined(__AROSEXEC_SMP__)
            if (mask)
                KrnGetCPUMask(i, mask);
#endif
        }
    }

#if defined(__AROSEXEC_SMP__)
    if (!found)
    {
        if (mask)
            KrnFreeCPUMask(mask);
        return EINVAL;
    }
    *maskptr = mask;
#endif

    return found ? 0 : EINVAL;
}

/*
 * The CPUs task may run on, as a set of setsize bytes. EINVAL if the
 * set is too small to hold every CPU the system has.
 */
int _pthread_task_to_cpuset(struct Task *task, size_t setsize, cpu_set_t *set)
{
    APTR KernelBase = OpenResource("kernel.resource");
    unsigned int i, count = KernelBase ? KrnGetCPUCount() : 1;

    if (!set || setsize < CPU_ALLOC_SIZE(count))
        return EINVAL;

    CPU_ZERO_S(setsize, set);

#if defined(__AROSEXEC_SMP__)
    if (KernelBase && task)
    {
        struct Library *TaskResBase = OpenResource("task.resource");
        APTR mask;

        if (TaskResBase && (mask = KrnAllocCPUMask()))
        {
            struct TagItem tags[] =
            {
                { TaskTag_CPUAffinity, (IPTR)mask },
                { TAG_DONE,            0          }
            };

            /* QueryTaskTagList() adds the task's CPUs to the mask */
            KrnClearCPUMask(mask);
            QueryTaskTagList(task, tags);
            for (i = 0; i < count; i++)
                if (KrnCPUInMask(i, mask))
                    CPU_SET_S(i, setsize, set);
            KrnFreeCPUMask(mask);

            return 0;
        }
        return ENOMEM;
    }
#else
    (void)task;
    (void)i;
#endif

    CPU_SET_S(0, setsize, set);

    return 0;
}

/* Free a mask from _pthread_cpuset_to_mask() that was not handed on */
void _pthread_free_affinity(APTR mask)
{
    if (mask)
    {
        APTR KernelBase = OpenResource("kernel.resource");

        KrnFreeCPUMask(mask);
    }
}
