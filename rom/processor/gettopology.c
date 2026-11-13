/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: GetCPUTopology() - Provides the system processor topology
*/

#include <aros/libcall.h>
#include <resources/processor.h>

#include "processor_intern.h"

/*****************************************************************************

    NAME */
#include <proto/processor.h>

        AROS_LH0(const struct ProcessorTopology *, GetCPUTopology,

/*  SYNOPSIS */

/*  LOCATION */
        struct ProcessorBase *, ProcessorBase, 2, Processor)

/*  FUNCTION

        Returns a description of how the system's logical processors are
        arranged into cores, clusters and packages.

    INPUTS

        None

    RESULT

        Pointer to a read-only struct ProcessorTopology owned by the
        resource, valid for the lifetime of the system, or NULL if no
        topology information could be gathered.

        pt_Entries holds one struct ProcessorTopologyEntry per logical
        processor; pte_LogicalID of an entry may be used as the core
        number for GetCoreInfo() and GCIT_SelectedProcessor.

    NOTES

        On hardware where a topology level does not exist the respective
        count is 1 and the IDs are 0.

    EXAMPLE

    BUGS

    SEE ALSO

        GetCPUInfo(), GetCoreInfo()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    return ProcessorBase->Topology;

    AROS_LIBFUNC_EXIT
} /* GetCPUTopology() */
