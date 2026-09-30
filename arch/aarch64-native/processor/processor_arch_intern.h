/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef PROCESSOR_ARCH_INTERN_H
#define PROCESSOR_ARCH_INTERN_H

#include <exec/types.h>

struct ProcessorBase;
struct TagItem;

struct ARMProcessorInformation
{
    ULONG           VendorID;
    CONST_STRPTR    Vendor;
    TEXT            BrandStringBuffer[48];
    STRPTR          BrandString;
    ULONG           Family;
    CONST_STRPTR    FamilyString;
    ULONG           Model;
    ULONG           VectorUnit;
    ULONG           Features1;

    /* KiB */
    ULONG           L1DataCacheSize;
    ULONG           L1InstructionCacheSize;
    ULONG           L2CacheSize;
    ULONG           CacheLineSize;  /* bytes */

    UQUAD           MaxCPUFrequency;
    UQUAD           CPUFrequency;

    /* 64-bit: Aff3 is above bit 32 */
    UQUAD           MPIDR;
};

VOID ReadProcessorInformation(struct ARMProcessorInformation * info);
VOID ReadMaxFrequencyInformation(struct ARMProcessorInformation * info);
UQUAD GetCurrentProcessorFrequency(struct ProcessorBase *ProcessorBase, struct ARMProcessorInformation * info);
VOID Processor_FillTopology(struct ProcessorBase * ProcessorBase);
VOID ARM_AnswerTag(struct ProcessorBase * ProcessorBase, ULONG coreNo, struct TagItem * tag);

/* Bit 31 is RES1, so a zeroed MPIDR means "never read". */
#define MPIDR_VALID(mpidr)      (((mpidr) & (1ULL << 31)) != 0)
#define MPIDR_MT(mpidr)         (((mpidr) & (1ULL << 24)) != 0)
#define MPIDR_AFF0(mpidr)       ((ULONG)((mpidr) & 0xFF))
#define MPIDR_AFF1(mpidr)       ((ULONG)(((mpidr) >> 8) & 0xFF))
#define MPIDR_AFF2(mpidr)       ((ULONG)(((mpidr) >> 16) & 0xFF))
#define MPIDR_AFF3(mpidr)       ((ULONG)(((mpidr) >> 32) & 0xFF))

/* Features1 bits */
#define FEATB_FPU               0
#define FEATF_FPU               (1 << FEATB_FPU)
#define FEATB_NEON              1
#define FEATF_NEON              (1 << FEATB_NEON)
#define FEATB_FPU_HALF          2
#define FEATF_FPU_HALF          (1 << FEATB_FPU_HALF)
#define FEATB_CRC32             3
#define FEATF_CRC32             (1 << FEATB_CRC32)
#define FEATB_AES               4
#define FEATF_AES               (1 << FEATB_AES)
#define FEATB_BIGEND            30
#define FEATF_BIGEND            (1 << FEATB_BIGEND)

#endif /* PROCESSOR_ARCH_INTERN_H */
