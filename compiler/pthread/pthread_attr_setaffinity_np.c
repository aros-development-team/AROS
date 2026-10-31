/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <errno.h>
#include <string.h>

#include "pthread_intern.h"
#include "debug.h"

/****************************************************************************
    NAME */
        int pthread_attr_setaffinity_np(

/*  SYNOPSIS */
        pthread_attr_t *attr,
        size_t cpusetsize,
        const cpu_set_t *cpuset)

/*  FUNCTION
        Set the CPUs that threads created with attr may run on.

    INPUTS
        attr - Initialized writable attribute object.
        cpusetsize - Size of *cpuset in bytes (sizeof(cpu_set_t), or
            CPU_ALLOC_SIZE() for a set of another size).
        cpuset - The CPUs, numbered as KrnGetCPUNumber() numbers them.

    RESULT
        0 on success; EINVAL if attr or cpuset is NULL, or if cpuset
        names no CPU that exists. Errors are returned directly.

    NOTES
        Include <pthread.h>; link with pthread. Non-portable (glibc). CPUs
        beyond those the system has are ignored. Without this call a new
        thread inherits its creator's affinity. On a uniprocessor build
        only CPU 0 exists.

    SEE ALSO
        pthread_attr_getaffinity_np(),
        pthread_setaffinity_np(),
        pthread_create()

*****************************************************************************/
{
    APTR mask;
    size_t n;
    int err;

    D(bug("%s(%p, %u, %p)\n", __FUNCTION__, attr, (unsigned)cpusetsize, cpuset));

    if (attr == NULL)
        return EINVAL;

    /* Validates the set; the mask itself is made by pthread_create() */
    if ((err = _pthread_cpuset_to_mask(cpusetsize, cpuset, &mask)) != 0)
        return err;
    _pthread_free_affinity(mask);

    n = cpusetsize < sizeof(cpu_set_t) ? cpusetsize : sizeof(cpu_set_t);
    CPU_ZERO(&attr->affinity);
    memcpy(&attr->affinity, cpuset, n);
    attr->affinityset = 1;

    return 0;
}
