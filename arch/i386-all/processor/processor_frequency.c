/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include "processor_arch_intern.h"

UQUAD GetCurrentProcessorFrequency(struct X86ProcessorInformation * info)
{
D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    /* Stub */
    return 0;
}

VOID ReadMaxFrequencyInformation(struct X86ProcessorInformation * info)
{
D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    /* Stub */
    info->MaxCPUFrequency = 0;
    info->MaxFSBFrequency = 0;
}
