/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <resources/processor.h>
#include <stdint.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

/* Tasks run at EL1t, so these sysreg reads need no SuperState(). */

static const char *ARMCPUVendors[] =
{
    "Unknown",
    "ARM Ltd.",
    "Broadcom",
    "Cavium",
    "DEC",
    "Fujitsu",
    "NVIDIA",
    "Applied Micro",
    "Qualcomm",
    "Marvell",
    "Intel",
    "Apple",
    NULL
};

static const struct
{
    UWORD        part;
    const char  *name;
} ARMCPUParts[] =
{
    { 0xD03, "Cortex-A53"  },   /* Pi 3 */
    { 0xD04, "Cortex-A35"  },
    { 0xD05, "Cortex-A55"  },
    { 0xD07, "Cortex-A57"  },
    { 0xD08, "Cortex-A72"  },   /* Pi 4 */
    { 0xD09, "Cortex-A73"  },
    { 0xD0A, "Cortex-A75"  },
    { 0xD0B, "Cortex-A76"  },   /* Pi 5 */
    { 0xD0C, "Neoverse-N1" },
    { 0xD41, "Cortex-A78"  },
    { 0xD44, "Cortex-X1"   },
    { 0, NULL }
};

/* No stdio in a kickstart resource. */
static STRPTR append_str(STRPTR d, STRPTR end, const char *s)
{
    while (*s && (d < end - 1))
        *d++ = *s++;
    *d = 0;

    return d;
}

static STRPTR append_dec(STRPTR d, STRPTR end, ULONG v)
{
    char tmp[12];
    int i = 0;

    do {
        tmp[i++] = '0' + (v % 10);
        v /= 10;
    } while (v && (i < (int)sizeof(tmp)));

    while (i-- && (d < end - 1))
        *d++ = tmp[i];
    *d = 0;

    return d;
}

static STRPTR append_hex(STRPTR d, STRPTR end, ULONG v)
{
    static const char digits[] = "0123456789abcdef";
    int shift = 8;

    while (shift >= 0)
    {
        if (d < end - 1)
            *d++ = digits[(v >> shift) & 0xF];
        shift -= 4;
    }
    *d = 0;

    return d;
}

/* CSSELR selects a cache by [3:1] = level - 1, [0] = 1 for instruction. */
static ULONG cache_size_kb(unsigned int level, unsigned int instruction)
{
    uint64_t ccsidr, mmfr2;
    uint64_t line_bytes, assoc, sets;

    __asm__ volatile("msr csselr_el1, %0" ::
        "r"((uint64_t)(((level - 1) << 1) | (instruction ? 1 : 0))));
    __asm__ volatile("isb");
    __asm__ volatile("mrs %0, ccsidr_el1" : "=r"(ccsidr));

    /* FEAT_CCIDX widens CCSIDR; MMFR2 reads as zero on ARMv8.0. */
    __asm__ volatile("mrs %0, id_aa64mmfr2_el1" : "=r"(mmfr2));

    line_bytes = 1ULL << ((ccsidr & 7) + 4);

    if (((mmfr2 >> 20) & 0xF) != 0)
    {
        assoc = ((ccsidr >> 3) & 0x1FFFFF) + 1;
        sets  = ((ccsidr >> 32) & 0xFFFFFF) + 1;
    }
    else
    {
        assoc = ((ccsidr >> 3) & 0x3FF) + 1;
        sets  = ((ccsidr >> 13) & 0x7FFF) + 1;
    }

    return (ULONG)((sets * assoc * line_bytes) >> 10);
}

VOID ReadProcessorInformation(struct ARMProcessorInformation * info)
{
    uint64_t midr, mpidr, pfr0, isar0, sctlr, ctr, clidr;
    ULONG part, variant, revision, ctype;
    const char *name = NULL;
    STRPTR d, end;
    int i;

    D(bug("[processor.AArch64] %s()\n", __PRETTY_FUNCTION__));

    __asm__ volatile("mrs %0, midr_el1"         : "=r"(midr));
    __asm__ volatile("mrs %0, mpidr_el1"        : "=r"(mpidr));
    __asm__ volatile("mrs %0, id_aa64pfr0_el1"  : "=r"(pfr0));
    __asm__ volatile("mrs %0, id_aa64isar0_el1" : "=r"(isar0));
    __asm__ volatile("mrs %0, sctlr_el1"        : "=r"(sctlr));
    __asm__ volatile("mrs %0, ctr_el0"          : "=r"(ctr));
    __asm__ volatile("mrs %0, clidr_el1"        : "=r"(clidr));

    info->MPIDR    = mpidr;
    info->VendorID = (ULONG)((midr >> 24) & 0xFF);
    part           = (ULONG)((midr >> 4) & 0xFFF);
    variant        = (ULONG)((midr >> 20) & 0xF);
    revision       = (ULONG)(midr & 0xF);

    info->Model = part;
    /* There is no CPUFAMILY_ARM_8; v7 is the last the resource names. */
    info->Family = CPUFAMILY_ARM_7;
    info->FamilyString = (CONST_STRPTR)"v8-A";

    /* 0xF in an ID_AA64PFR0 field means the feature is not implemented. */
    if (((pfr0 >> 16) & 0xF) != 0xF)
        info->Features1 |= FEATF_FPU;
    if (((pfr0 >> 20) & 0xF) != 0xF)
        info->Features1 |= FEATF_NEON;
    if (((pfr0 >> 16) & 0xF) == 1)
        info->Features1 |= FEATF_FPU_HALF;

    if (((isar0 >> 16) & 0xF) != 0)
        info->Features1 |= FEATF_CRC32;
    if (((isar0 >> 4) & 0xF) != 0)
        info->Features1 |= FEATF_AES;

    if (sctlr & (1ULL << 25))           /* SCTLR_EL1.EE */
        info->Features1 |= FEATF_BIGEND;

    /* CTR_EL0.DminLine counts words, not bytes. */
    info->CacheLineSize = 4UL << ((ctr >> 16) & 0xF);

    /* CLIDR Ctype: 1 = I only, 2 = D only, 3 = separate, 4 = unified. */
    ctype = (ULONG)(clidr & 7);
    if (ctype >= 2)
        info->L1DataCacheSize = cache_size_kb(1, 0);
    if ((ctype == 1) || (ctype == 3))
        info->L1InstructionCacheSize = cache_size_kb(1, 1);

    ctype = (ULONG)((clidr >> 3) & 7);
    if (ctype)
        info->L2CacheSize = cache_size_kb(2, 0);

    switch (info->VendorID)
    {
        case 'A': info->Vendor = ARMCPUVendors[1];  break;
        case 'B': info->Vendor = ARMCPUVendors[2];  break;
        case 'C': info->Vendor = ARMCPUVendors[3];  break;
        case 'D': info->Vendor = ARMCPUVendors[4];  break;
        case 'F': info->Vendor = ARMCPUVendors[5];  break;
        case 'N': info->Vendor = ARMCPUVendors[6];  break;
        case 'P': info->Vendor = ARMCPUVendors[7];  break;
        case 'Q': info->Vendor = ARMCPUVendors[8];  break;
        case 'V': info->Vendor = ARMCPUVendors[9];  break;
        case 'i': info->Vendor = ARMCPUVendors[10]; break;
        case 'a': info->Vendor = ARMCPUVendors[11]; break;
        default:  info->Vendor = ARMCPUVendors[0];  break;
    }

    for (i = 0; ARMCPUParts[i].name; i++)
    {
        if (ARMCPUParts[i].part == part)
        {
            name = ARMCPUParts[i].name;
            break;
        }
    }

    d   = info->BrandStringBuffer;
    end = info->BrandStringBuffer + sizeof(info->BrandStringBuffer);

    if (name)
        d = append_str(d, end, name);
    else
    {
        d = append_str(d, end, "ARMv8 part 0x");
        d = append_hex(d, end, part);
    }

    d = append_str(d, end, " r");
    d = append_dec(d, end, variant);
    d = append_str(d, end, "p");
    d = append_dec(d, end, revision);

    info->BrandString = info->BrandStringBuffer;

    D(bug("[processor.AArch64] %s: %s, L1 %uK/%uK L2 %uK, line %u\n",
        __PRETTY_FUNCTION__, info->BrandString, info->L1DataCacheSize,
        info->L1InstructionCacheSize, info->L2CacheSize, info->CacheLineSize));
}

/* Aff1 = cluster, Aff0 = core; with MT set Aff0 is the thread. */
VOID Processor_FillTopology(struct ProcessorBase * ProcessorBase)
{
    struct ARMProcessorInformation **sysprocs = ProcessorBase->Private1;
    struct ProcessorTopology *topo = ProcessorBase->Topology;
    struct ProcessorTopologyEntry *entries;
    ULONG i, j;
    ULONG clusters = 0, cores = 0, tpc = 1;

    if (!topo)
        return;
    entries = (struct ProcessorTopologyEntry *)topo->pt_Entries;

    for (i = 0; i < topo->pt_Count; i++)
    {
        UQUAD mpidr = sysprocs[i] ? sysprocs[i]->MPIDR : 0;

        entries[i].pte_PackageID = 0;
        if (MPIDR_VALID(mpidr))
        {
            entries[i].pte_PhysicalID = (ULONG)(mpidr & 0x00FFFFFF);
            if (MPIDR_MT(mpidr))
            {
                entries[i].pte_ClusterID = MPIDR_AFF2(mpidr);
                entries[i].pte_CoreID = MPIDR_AFF1(mpidr);
                entries[i].pte_ThreadID = MPIDR_AFF0(mpidr);
            }
            else
            {
                entries[i].pte_ClusterID = MPIDR_AFF1(mpidr);
                entries[i].pte_CoreID = MPIDR_AFF0(mpidr);
                entries[i].pte_ThreadID = 0;
            }
        }
    }

    for (i = 0; i < topo->pt_Count; i++)
    {
        ULONG threads = 1;
        BOOL firstofcluster = TRUE, firstofcore = TRUE;

        for (j = 0; j < i; j++)
        {
            if (entries[j].pte_ClusterID == entries[i].pte_ClusterID)
            {
                firstofcluster = FALSE;
                if (entries[j].pte_CoreID == entries[i].pte_CoreID)
                    firstofcore = FALSE;
            }
        }
        if (firstofcluster)
            clusters++;
        if (firstofcore)
        {
            cores++;
            for (j = 0; j < topo->pt_Count; j++)
            {
                if (j != i &&
                    entries[j].pte_ClusterID == entries[i].pte_ClusterID &&
                    entries[j].pte_CoreID == entries[i].pte_CoreID)
                    threads++;
            }
            if (threads > tpc)
                tpc = threads;
        }
    }

    topo->pt_Packages = 1;
    topo->pt_Clusters = clusters ? clusters : 1;
    topo->pt_Cores = cores ? cores : topo->pt_Count;
    topo->pt_ThreadsPerCore = tpc;
}
