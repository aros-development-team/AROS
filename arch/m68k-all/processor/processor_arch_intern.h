/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROCESSOR_ARCH_INTERN_H
#define PROCESSOR_ARCH_INTERN_H

#include <exec/types.h>

struct M68KProcessorInformation
{
    TEXT    BrandStringBuffer[48];
    STRPTR  BrandString;
    ULONG   CPUModel;
    ULONG   FPUModel;

    /* Processor cache */
    ULONG   L1DataCacheSize;
    ULONG   L1InstructionCacheSize;
    
    /* Frequency information */
    UQUAD   CPUFrequency;
};

struct SystemProcessors
{
    struct M68KProcessorInformation processor;
};

#define FPUTYPE_UNKNOWN     0
#define FPUTYPE_NONE        1
#define FPUTYPE_68881       2
#define FPUTYPE_68882       3
#define FPUTYPE_INTERNAL    4

#endif /* PROCESSOR_ARCH_INTERN_H */
