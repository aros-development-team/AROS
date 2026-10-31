/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <proto/exec.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#include <proto/task.h>

#include <errno.h>

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_setaffinity_np(

/*  SYNOPSIS */
        pthread_t thread,
        size_t cpusetsize,
        const cpu_set_t *cpuset)

/*  FUNCTION
        Set the CPUs a running thread may run on.

    INPUTS
        thread - Registered pthread identifier (pthread_self() for the
            caller).
        cpusetsize - Size of *cpuset in bytes.
        cpuset - The CPUs, numbered as KrnGetCPUNumber() numbers them.

    RESULT
        0 on success; ESRCH if thread is not a live thread; EINVAL if
        cpuset is NULL or names no CPU that exists; ENOMEM if no CPU mask
        could be allocated. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. Non-portable (glibc). Uses
        task.resource SetTaskAffinity(): a thread running on a CPU it may
        no longer use moves at that CPU's next scheduling point. Threads
        it creates afterwards inherit the new set. On a uniprocessor
        build only CPU 0 exists.

    SEE ALSO
        pthread_getaffinity_np(),
        pthread_attr_setaffinity_np(),
        sched_getcpu()

*****************************************************************************/
{
    ThreadInfo *inf;
    struct Task *task;
    APTR mask;
    int err;

    D(bug("%s(%u, %u, %p)\n", __FUNCTION__, thread, (unsigned)cpusetsize, cpuset));

    inf = GetThreadInfo(thread);
    if (inf == NULL || (task = inf->task) == NULL || inf->finished)
        return ESRCH;

    if ((err = _pthread_cpuset_to_mask(cpusetsize, cpuset, &mask)) != 0)
        return err;

#if defined(__AROSEXEC_SMP__)
    if (mask)
    {
        APTR KernelBase = OpenResource("kernel.resource");
        struct Library *TaskResBase = OpenResource("task.resource");
        BOOL ok = TaskResBase && SetTaskAffinity(task, mask);

        /* SetTaskAffinity() copies the mask */
        KrnFreeCPUMask(mask);
        if (!ok)
            return EINVAL;
    }
#endif

    return 0;
}
