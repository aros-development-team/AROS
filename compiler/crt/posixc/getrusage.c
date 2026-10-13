/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function getrusage().
*/

#include <errno.h>
#include <sys/resource.h>

#include <exec/tasks.h>
#include <utility/tagitem.h>
#include <resources/task.h>

#include <proto/exec.h>
#include <proto/task.h>   /* QueryTaskTagList(); TaskResBase is the module global genmodule opens */

extern APTR TaskResBase;

/*****************************************************************************

    NAME */

        int getrusage (

/*  SYNOPSIS */
        int who,
        struct rusage *usage)

/*  FUNCTION
        Get resource usage information for the calling process.

    INPUTS
        who   - RUSAGE_SELF (RUSAGE_CHILDREN is refused: AROS keeps no
                child accounting)
        usage - filled with the accounting data

    RESULT
        0 on success, -1 with errno on error.

    NOTES
        ru_utime is the calling task's CPU time, read from task.resource
        (TaskTag_CPUTime). AROS has no user/system time split, so ru_stime
        is 0. The fourteen BSD extension fields (ru_maxrss through
        ru_nivcsw) are zero: AROS tracks none of them. Zeroing documented
        untracked extensions is exactly what Linux does for fields it does
        not maintain; the POSIX fields are real.

        The timeval task.resource returns is normalised here: its rounding
        ((tv_nsec + 500) / 1000) can yield tv_usec == 1000000, which is not
        a valid timeval.

    EXAMPLE

    BUGS
        RUSAGE_SELF covers the calling task only. POSIX requires it to
        cover all threads of the process, but posixc threads are separate
        exec tasks (see pthread_create()) with no per-process enumeration
        facility posixc could walk, so other threads' CPU time is not
        included. Whether task.resource should grow a process-wide query
        is an open question.

        RUSAGE_CHILDREN (and any other who value) fails with EINVAL: AROS
        has no waited-for children's CPU time source (wait()/waitpid()
        collect none). If child accounting ever lands, revisit.

    SEE ALSO
        getrlimit()

    INTERNALS
        The task.resource handle is the genmodule-opened TaskResBase
        global, not a per-call OpenResource("task.resource") as in
        clock_gettime(CLOCK_THREAD_CPUTIME_ID). genmodule opens it once
        at posixc.library Init (tools/genmodule/writestart.c; posixc.conf
        selects pertaskbase, which implies dupbase), so a per-call lookup
        would repeat a Forbid/FindName list walk on every call for the
        same pointer; the global is set before any vector is callable,
        and library Init itself fails when the resource is absent, so the
        NULL check below is unreachable defense-in-depth. Both readers
        query TaskTag_CPUTime for FindTask(NULL), hence identical values.

******************************************************************************/
{
    struct TagItem tags[2];

    if (usage == NULL)
    {
        errno = EFAULT;
        return -1;
    }

    /* No child accounting exists: refuse anything but RUSAGE_SELF rather
       than return zero-filled fake data. */
    if (who != RUSAGE_SELF)
    {
        errno = EINVAL;
        return -1;
    }

    /* task.resource is opened for posixc.library by genmodule; if some
       target ever lacks it, only that target fails, and honestly. */
    if (TaskResBase == NULL)
    {
        errno = ENOSYS;
        return -1;
    }

    tags[0].ti_Tag  = TaskTag_CPUTime;
    tags[0].ti_Data = (IPTR)&usage->ru_utime;
    tags[1].ti_Tag  = TAG_DONE;
    QueryTaskTagList(FindTask(NULL), tags);

    /* Normalise: task.resource rounds (tv_nsec + 500) / 1000, which yields
       tv_usec == 1000000 for tv_nsec >= 999999500. */
    while (usage->ru_utime.tv_usec >= 1000000)
    {
        usage->ru_utime.tv_sec++;
        usage->ru_utime.tv_usec -= 1000000;
    }

    /* No user/system split on AROS. */
    usage->ru_stime.tv_sec  = 0;
    usage->ru_stime.tv_usec = 0;

    /* Untracked BSD extensions: documented zeros, never fakes. */
    usage->ru_maxrss  = 0;
    usage->ru_ixrss   = 0;
    usage->ru_idrss   = 0;
    usage->ru_isrss   = 0;
    usage->ru_minflt  = 0;
    usage->ru_majflt  = 0;
    usage->ru_nswap   = 0;
    usage->ru_inblock = 0;
    usage->ru_oublock = 0;
    usage->ru_msgsnd  = 0;
    usage->ru_msgrcv  = 0;
    usage->ru_nsignals = 0;
    usage->ru_nvcsw   = 0;
    usage->ru_nivcsw  = 0;

    return 0;
}
