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
#define FEATB_THUMB             0
#define FEATF_THUMB             (1 << FEATB_THUMB)
#define FEATB_THUMBEX           1
#define FEATF_THUMBEX           (1 << FEATB_THUMBEX)
#define FEATB_BRANCHP           2
#define FEATF_BRANCHP           (1 << FEATB_BRANCHP)
#define FEATB_FPU               3
#define FEATF_FPU               (1 << FEATB_FPU)
#define FEATB_FPU_VFP           FEATB_FPU
#define FEATF_FPU_VFP           (1 << FEATB_FPU_VFP)
#define FEATB_FPU_VFP2          4
#define FEATF_FPU_VFP2          (1 << FEATB_FPU_VFP3)
#define FEATB_FPU_VFP3          5
#define FEATF_FPU_VFP3          (1 << FEATB_FPU_VFP3)
#define FEATB_FPU_VFP3_16       6
#define FEATF_FPU_VFP3_16       (1 << FEATB_FPU_VFP3_16)
#define FEATB_NEON              7
#define FEATF_NEON              (1 << FEATB_NEON)
#define FEATB_FPU_VFP4          8
#define FEATF_FPU_VFP4          (1 << FEATB_FPU_VFP4)
#define FEATB_SECURE            29
#define FEATF_SECURE            (1 << FEATB_SECURE)
#define FEATB_BIGEND            30
#define FEATF_BIGEND            (1 << FEATB_BIGEND)
#endif /* PROCESSOR_ARCH_INTERN_H */
