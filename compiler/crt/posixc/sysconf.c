/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function sysconf().
*/

#include <errno.h>
#include <limits.h>
#include <exec/memory.h>
#include <resources/processor.h>
#include <proto/exec.h>
#include <proto/processor.h>

/*****************************************************************************

    NAME */
#include <time.h>
#include <unistd.h>

        long sysconf(

/*  SYNOPSIS */
        int name)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES
        Only _SC_ARG_MAX, _SC_PAGESIZE/_SC_PAGE_SIZE, _SC_CLK_TCK and the
        _SC_PHYS_PAGES, _SC_NPROCESSORS_CONF and _SC_NPROCESSORS_ONLN
        extensions are implemented; other names fail with EINVAL.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    switch (name)
    {
        /* TODO: Implement other names */
        case _SC_ARG_MAX: return ARG_MAX;

        case _SC_PAGESIZE: /* same value expected for _SC_PAGE_SIZE */
        case _SC_PAGE_SIZE:
            return 4096;

        case _SC_PHYS_PAGES:
            /* Total system memory in _SC_PAGESIZE units: exec's MEMF_TOTAL
               size of all memory regions, which does not change at run
               time. Mesa's os_get_total_physical_memory() multiplies it back
               by the page size (lavapipe reports it as its heap size, which
               must be the same on every query). */
            return (long)(AvailMem(MEMF_ANY | MEMF_TOTAL) / 4096);

        case _SC_CLK_TCK:
            /* What times() and clock() count in; CPython refuses to start
               when this query fails. */
            return CLOCKS_PER_SEC;

        case _SC_NPROCESSORS_CONF:
        case _SC_NPROCESSORS_ONLN:
        {
            /* CPUs are not brought on- or offline at run time, so both are
               the count processor.resource reports. */
            APTR ProcessorBase = OpenResource(PROCESSORNAME);
            ULONG count = 0;

            if (ProcessorBase)
                GetCPUInfoTags(GCIT_NumberOfProcessors, (IPTR)&count, TAG_DONE);
            return count ? (long)count : 1;
        }

        default:
            errno = EINVAL;
            return -1;
    };
} /* sysconf */
