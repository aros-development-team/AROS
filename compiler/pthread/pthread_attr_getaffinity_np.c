/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>
#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_attr_getaffinity_np(

/*  SYNOPSIS */
        const pthread_attr_t *attr,
        size_t cpusetsize,
        cpu_set_t *cpuset)

/*  FUNCTION
        Return the CPUs that threads created with attr may run on.

    INPUTS
        attr - Initialized attribute object.
        cpusetsize - Size of *cpuset in bytes.
        cpuset - Receives the CPUs.

    RESULT
        0 on success; EINVAL if attr or cpuset is NULL, or if a set made
        with pthread_attr_setaffinity_np() does not fit in cpusetsize
        bytes. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. Non-portable (glibc). If no
        set was given with pthread_attr_setaffinity_np(), every CPU is
        returned (the thread then inherits its creator's affinity).

    SEE ALSO
        pthread_attr_setaffinity_np(),
        pthread_getaffinity_np()

*****************************************************************************/
{
    D(bug("%s(%p, %u, %p)\n", __FUNCTION__, attr, (unsigned)cpusetsize, cpuset));

    if (attr == NULL || cpuset == NULL)
        return EINVAL;

    if (attr->affinityset)
    {
        size_t i;

        /* Every CPU in the stored set must fit in the caller's */
        for (i = cpusetsize * 8; i < CPU_SETSIZE; i++)
            if (CPU_ISSET(i, &attr->affinity))
                return EINVAL;
        CPU_ZERO_S(cpusetsize, cpuset);
        memcpy(cpuset, &attr->affinity,
               cpusetsize < sizeof(cpu_set_t) ? cpusetsize : sizeof(cpu_set_t));
    }
    else
        memset(cpuset, 0xff, cpusetsize);

    return 0;
}
