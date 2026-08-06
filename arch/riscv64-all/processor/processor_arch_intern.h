/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef PROCESSOR_ARCH_INTERN_H
#define PROCESSOR_ARCH_INTERN_H

#include <exec/types.h>
#include <utility/tagitem.h>

#include "processor_intern.h"

/* ISA extension bits, decoded from the device tree's isa string */
#define RVFEATB_M       0   /* Integer multiply/divide  */
#define RVFEATB_A       1   /* Atomics                  */
#define RVFEATB_F       2   /* Single precision float   */
#define RVFEATB_D       3   /* Double precision float   */
#define RVFEATB_C       4   /* Compressed instructions  */
#define RVFEATB_V       5   /* Vector                   */
#define RVFEATB_H       6   /* Hypervisor               */
#define RVFEATB_ZBA     7
#define RVFEATB_ZBB     8
#define RVFEATB_ZBC     9
#define RVFEATB_ZBS     10
#define RVFEATB_ZFH     11
#define RVFEATB_ZICBOM  12
#define RVFEATB_ZICBOZ  13
#define RVFEATB_SSTC    14
#define RVFEATB_SVPBMT  15

#define RVFEATF(b)      (1UL << (b))

struct RiscVProcessorInformation
{
    ULONG   HartID;
    ULONG   Features;               /* RVFEATF() bits           */
    ULONG   Family;                 /* CPUFAMILY_RISCV_RVxx     */
    ULONG   VectorUnit;             /* VECTORTYPE_xxx           */
    ULONG   L1DataCacheSize;        /* kB                       */
    ULONG   L1InstructionCacheSize; /* kB                       */
    ULONG   L2CacheSize;            /* kB                       */
    ULONG   L3CacheSize;            /* kB                       */
    ULONG   CacheLineSize;          /* bytes, smallest of all   */
    UQUAD   ClockFrequency;         /* Hz, 0 when not described */
    UQUAD   VendorID;               /* mvendorid                */
    UQUAD   ArchID;                 /* marchid                  */
    UQUAD   ImplID;                 /* mimpid                   */
    CONST_STRPTR ISAString;
    CONST_STRPTR ModelString;
};

/* Private1 points to an array of cpucount of these */
static inline struct RiscVProcessorInformation *
Processor_GetInfo(struct ProcessorBase *ProcessorBase, ULONG core)
{
    struct RiscVProcessorInformation *info = ProcessorBase->Private1;

    return info ? &info[core] : NULL;
}

/* Answer one per-core tag (getcpuinfo.c and getcoreinfo.c share this) */
VOID Processor_AnswerTag(struct ProcessorBase *ProcessorBase, ULONG coreNo,
                         struct TagItem *tag);

/*
 * The machine ID CSRs can not be read from S-mode; how they are
 * obtained is the platform's business (SBI on an OpenSBI machine). The
 * weak default reports them unknown.
 */
VOID Processor_PlatformReadIDs(UQUAD *vendor, UQUAD *archid, UQUAD *impl);

#endif /* PROCESSOR_ARCH_INTERN_H */
