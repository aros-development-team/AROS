/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROCESSOR_ARCH_INTERN_H
#define PROCESSOR_ARCH_INTERN_H

#include <exec/types.h>

struct ARMProcessorInformation
{
    char    *VendorID;
    ULONG   Vendor;
    TEXT    BrandStringBuffer[48];
    STRPTR  BrandString;
    ULONG   Family;
    ULONG   Model;
    ULONG   VectorUnit;
    ULONG   Features1;

    /* Processor cache */
    ULONG   L1DataCacheSize;
    ULONG   L1InstructionCacheSize;
    ULONG   L2CacheSize;
    ULONG   CacheLineSize;  /* Min. of L1, L2 */

    
    /* Frequency information */
    UQUAD   MaxCPUFrequency;
};

VOID ReadProcessorInformation(struct ARMProcessorInformation * info);
VOID ReadMaxFrequencyInformation(struct ARMProcessorInformation * info);
UQUAD GetCurrentProcessorFrequency(struct ARMProcessorInformation * info);

/* Flags */
#define FEATB_FPU   0

#define FEATF_FPU   (1 << FEATB_FPU)


#endif /* PROCESSOR_ARCH_INTERN_H */
