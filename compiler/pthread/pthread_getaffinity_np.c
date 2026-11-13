/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_getaffinity_np(

/*  SYNOPSIS */
        pthread_t thread,
        size_t cpusetsize,
        cpu_set_t *cpuset)

/*  FUNCTION
        Return the CPUs a running thread may run on.

    INPUTS
        thread - Registered pthread identifier (pthread_self() for the
            caller).
        cpusetsize - Size of *cpuset in bytes.
        cpuset - Receives the CPUs, numbered as KrnGetCPUNumber() numbers
            them.

    RESULT
        0 on success; ESRCH if thread is not a live thread; EINVAL if
        cpuset is NULL or too small for the system's CPUs; ENOMEM if no
        CPU mask could be allocated. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. Non-portable (glibc). On a
        uniprocessor build the set is always CPU 0.

    SEE ALSO
        pthread_setaffinity_np(),
        pthread_attr_getaffinity_np(),
        sched_getcpu()

*****************************************************************************/
{
    ThreadInfo *inf;
    struct Task *task;

    D(bug("%s(%u, %u, %p)\n", __FUNCTION__, thread, (unsigned)cpusetsize, cpuset));

    inf = GetThreadInfo(thread);
    if (inf == NULL || (task = inf->task) == NULL || inf->finished)
        return ESRCH;

    return _pthread_task_to_cpuset(task, cpusetsize, cpuset);
}
