/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/



#include "processor_arch_intern.h"

UQUAD GetCurrentProcessorFrequency(struct X86ProcessorInformation * info)
{
    /* Stub */
    return 0;
}

VOID ReadMaxFrequencyInformation(struct X86ProcessorInformation * info)
{
    /* Stub */
    info->MaxCPUFrequency = 0;
    info->MaxFSBFrequency = 0;
}
