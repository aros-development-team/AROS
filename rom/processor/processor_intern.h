/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for processor.resource
    Lang: english
*/

#ifndef PROCESSOR_INTERN_H
#define PROCESSOR_INTERN_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif
#ifndef UTILITY_UTILITY_H
#include <utility/utility.h>
#endif

struct ProcessorBase
{
    struct Library pb_LibNode;
    unsigned int cpucount;
    APTR kernelBase;
    APTR Private1;    /* Pointer to arch-specific implementation data */
};

#define KernelBase ProcessorBase->kernelBase

#endif /* PROCESSOR_INTERN_H */
