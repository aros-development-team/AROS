/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GetCoreInfo() - Provides information about a single core
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/processor.h>

#include "defaults.h"
#include "processor_intern.h"

/*****************************************************************************

    NAME */
#include <proto/processor.h>

        AROS_LH2(BOOL, GetCoreInfo,

/*  SYNOPSIS */
        AROS_LHA(ULONG, coreNo, D0),
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct ProcessorBase *, ProcessorBase, 3, Processor)

/*  FUNCTION

        Provides information about the given logical processor. Data is
        returned for each tag passed; see resources/processor.h for the
        per-core tags and GetCPUInfo() for their individual meanings.

    INPUTS

        coreNo  - logical processor to describe, 0 based. Valid values
                  are 0 .. GCIT_NumberOfProcessors - 1, and match the
                  pte_LogicalID fields of the GetCPUTopology() table.

        tagList - array of tags to fill in.

    TAGS

        GCIT_PackageID, GCIT_ClusterID, GCIT_CoreID, GCIT_ThreadID,
        GCIT_PhysicalID - (ULONG *) The position of this logical
                          processor in the system topology, and its
                          hardware ID (APIC ID / MPIDR / hart id).

        All per-core tags accepted by GetCPUInfo() are also accepted
        here.

    RESULT

        FALSE if coreNo does not name a present logical processor - the
        tags are then left untouched - TRUE otherwise.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        GetCPUInfo(), GetCPUTopology()

    INTERNALS

        This default implementation answers through GetCPUInfo() with a
        GCIT_SelectedProcessor control tag, so an architecture that only
        implements GetCPUInfo() still answers per-core queries with its
        real data.

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem prefix[] =
    {
        { GCIT_SelectedProcessor, (IPTR)coreNo  },
        { TAG_MORE,               (IPTR)tagList }
    };

    if (coreNo >= ProcessorBase->cpucount)
        return FALSE;

    AROS_LC1NR(void, GetCPUInfo,
        AROS_LCA(struct TagItem *, prefix, A0),
        struct ProcessorBase *, ProcessorBase, 1, Processor);

    return TRUE;

    AROS_LIBFUNC_EXIT
} /* GetCoreInfo() */
