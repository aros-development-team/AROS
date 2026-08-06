/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Gather what the device tree says about the harts.

    Everything a RISC-V machine tells the world about its processors is
    in the device tree: the /cpus/cpu@N nodes carry the isa string, the
    caches and the clock, and /cpus/cpu-map arranges the harts into
    threads, cores, clusters and sockets. The machine ID registers are
    M-mode CSRs and come from the platform (see Processor_PlatformReadIDs).
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/utility.h>
#include <resources/processor.h>

#include <string.h>

#include <fdt.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

static const struct
{
    const char *name;
    ULONG flags;
} RVExtensions[] =
{
    { "m",      RVFEATF(RVFEATB_M) },
    { "a",      RVFEATF(RVFEATB_A) },
    { "f",      RVFEATF(RVFEATB_F) },
    { "d",      RVFEATF(RVFEATB_D) },
    { "c",      RVFEATF(RVFEATB_C) },
    { "v",      RVFEATF(RVFEATB_V) },
    { "h",      RVFEATF(RVFEATB_H) },
    { "g",      RVFEATF(RVFEATB_M) | RVFEATF(RVFEATB_A) |
                RVFEATF(RVFEATB_F) | RVFEATF(RVFEATB_D) },
    { "b",      RVFEATF(RVFEATB_ZBA) | RVFEATF(RVFEATB_ZBB) |
                RVFEATF(RVFEATB_ZBS) },
    { "zba",    RVFEATF(RVFEATB_ZBA) },
    { "zbb",    RVFEATF(RVFEATB_ZBB) },
    { "zbc",    RVFEATF(RVFEATB_ZBC) },
    { "zbs",    RVFEATF(RVFEATB_ZBS) },
    { "zfh",    RVFEATF(RVFEATB_ZFH) },
    { "zicbom", RVFEATF(RVFEATB_ZICBOM) },
    { "zicboz", RVFEATF(RVFEATB_ZICBOZ) },
    { "sstc",   RVFEATF(RVFEATB_SSTC) },
    { "svpbmt", RVFEATF(RVFEATB_SVPBMT) },
    { NULL,     0 }
};

static ULONG ExtensionFlags(const char *word, ULONG len)
{
    ULONG i;

    for (i = 0; RVExtensions[i].name; i++)
    {
        if (strlen(RVExtensions[i].name) == len &&
            !strncmp(RVExtensions[i].name, word, len))
            return RVExtensions[i].flags;
    }
    return 0;
}

static STRPTR DupStr(CONST_STRPTR s)
{
    STRPTR copy;
    ULONG len;

    if (!s)
        return NULL;
    len = strlen(s);
    copy = AllocVec(len + 1, MEMF_ANY);
    if (copy)
        CopyMem((APTR)s, copy, len + 1);
    return copy;
}

/*
 * "rv64imafdcv_zba_zbb...": the base, then one letter per classic
 * extension, then underscore-separated multi-letter extensions.
 */
static void ParseISA(struct RiscVProcessorInformation *info, CONST_STRPTR isa)
{
    const char *p = isa;

    if (strlen(isa) < 5 || (p[0] != 'r' && p[0] != 'R'))
        return;

    if (p[2] == '6')
        info->Family = CPUFAMILY_RISCV_RV64;
    else if (p[2] == '3')
        info->Family = CPUFAMILY_RISCV_RV32;

    for (p = isa + 4; *p && *p != '_'; p++)
    {
        char c = (*p >= 'A' && *p <= 'Z') ? *p + ('a' - 'A') : *p;

        /* Multi-letter names end the single-letter section even when
           the underscore before them was left out */
        if (c == 'z' || c == 's' || c == 'x')
            break;
        if (c == 'i' || c == 'e')
            continue;
        info->Features |= ExtensionFlags(&c, 1);
    }

    while (*p)
    {
        const char *word;

        while (*p == '_')
            p++;
        word = p;
        while (*p && *p != '_')
            p++;
        if (p != word)
            info->Features |= ExtensionFlags(word, p - word);
    }
}

static fdt_node_t FindByPhandle(ULONG ph)
{
    fdt_node_t node;

    if (!ph)
        return FDT_NONE;
    for (node = FDT_Root(); node != FDT_NONE; node = FDT_NextNode(node))
    {
        if (FDT_GetPropU32(node, "phandle", 0) == ph ||
            FDT_GetPropU32(node, "linux,phandle", 0) == ph)
            return node;
    }
    return FDT_NONE;
}

static fdt_node_t FindByName(CONST_STRPTR name)
{
    fdt_node_t node;

    for (node = FDT_Root(); node != FDT_NONE; node = FDT_NextNode(node))
    {
        CONST_STRPTR nname = FDT_NodeName(node);

        if (nname && !strcmp(nname, name))
            return node;
    }
    return FDT_NONE;
}

static void ParseCaches(struct RiscVProcessorInformation *info, fdt_node_t cpu)
{
    fdt_node_t next;
    ULONG line, v, depth;

    info->L1InstructionCacheSize = FDT_GetPropU32(cpu, "i-cache-size", 0) / 1024;
    info->L1DataCacheSize = FDT_GetPropU32(cpu, "d-cache-size", 0) / 1024;

    line = FDT_GetPropU32(cpu, "d-cache-block-size", 0);
    v = FDT_GetPropU32(cpu, "i-cache-block-size", 0);
    if (v && (!line || v < line))
        line = v;

    next = FindByPhandle(FDT_GetPropU32(cpu, "next-level-cache", 0));
    for (depth = 2; next != FDT_NONE && depth <= 4; depth++)
    {
        ULONG size = FDT_GetPropU32(next, "cache-size", 0) / 1024;
        ULONG level = FDT_GetPropU32(next, "cache-level", depth);

        v = FDT_GetPropU32(next, "cache-block-size", 0);
        if (v && (!line || v < line))
            line = v;

        if (level >= 3)
            info->L3CacheSize = size;
        else
            info->L2CacheSize = size;

        next = FindByPhandle(FDT_GetPropU32(next, "next-level-cache", 0));
    }

    info->CacheLineSize = line;
}

/*
 * The isa string was split into riscv,isa-base and a string list of
 * riscv,isa-extensions in later bindings; some trees only carry the
 * new form. Rebuild a printable string and the feature bits from it.
 */
static void ParseISAExtensions(struct RiscVProcessorInformation *info,
                               fdt_node_t cpu)
{
    CONST_STRPTR base = FDT_GetPropStr(cpu, "riscv,isa-base");
    const char *ext;
    STRPTR out;
    ULONG len, total;

    if (base)
    {
        if (strlen(base) >= 5)
            info->Family = (base[2] == '6') ? CPUFAMILY_RISCV_RV64
                                            : CPUFAMILY_RISCV_RV32;
    }

    ext = FDT_GetProp(cpu, "riscv,isa-extensions", &len);
    if (!ext || !len)
        return;

    /* One flag lookup per list entry */
    {
        const char *p = ext;

        while (p < ext + len)
        {
            ULONG wlen = strlen(p);

            info->Features |= ExtensionFlags(p, wlen);
            p += wlen + 1;
        }
    }

    /* And one printable string: base, then each extension, '_'-joined */
    total = (base ? strlen(base) : 4) + 1;
    {
        const char *p = ext;

        while (p < ext + len)
        {
            total += strlen(p) + 1;
            p += strlen(p) + 1;
        }
    }
    out = AllocVec(total, MEMF_ANY);
    if (!out)
        return;
    strcpy((char *)out, base ? (const char *)base
        : (info->Family == CPUFAMILY_RISCV_RV32 ? "rv32" : "rv64"));
    {
        const char *p = ext;

        while (p < ext + len)
        {
            strcat((char *)out, "_");
            strcat((char *)out, p);
            p += strlen(p) + 1;
        }
    }
    info->ISAString = out;
}

static void ParseCPUNode(struct RiscVProcessorInformation *info,
                         fdt_node_t cpu, ULONG index)
{
    CONST_STRPTR str;
    CONST_APTR prop;
    ULONG len;

    /* "reg" is the hart id, in as many cells as /cpus declares */
    prop = FDT_GetProp(cpu, "reg", &len);
    if (prop && len >= 4)
        info->HartID = (ULONG)FDT_ReadCells(prop, len / 4);
    else
        info->HartID = index;

    str = FDT_GetPropStr(cpu, "riscv,isa");
    if (str)
    {
        info->ISAString = DupStr(str);
        ParseISA(info, str);
    }
    else
        ParseISAExtensions(info, cpu);

    if (info->Features & RVFEATF(RVFEATB_V))
        info->VectorUnit = VECTORTYPE_RVV;

    prop = FDT_GetProp(cpu, "clock-frequency", &len);
    if (prop)
        info->ClockFrequency = FDT_ReadCells(prop, len / 4);

    /* The specific part name leads the compatible list; "riscv" alone
       says nothing worth repeating */
    prop = FDT_GetProp(cpu, "compatible", &len);
    if (prop && strcmp(prop, "riscv"))
        info->ModelString = DupStr(prop);

    ParseCaches(info, cpu);
}

static void AssignTopology(struct ProcessorTopologyEntry *entries,
                           const fdt_node_t *cpunodes, ULONG ncpu, ULONG ph,
                           ULONG pkg, ULONG cluster, ULONG core, ULONG thread)
{
    fdt_node_t cpu = FindByPhandle(ph);
    ULONG i;

    if (cpu == FDT_NONE)
        return;
    for (i = 0; i < ncpu; i++)
    {
        if (cpunodes[i] == cpu)
        {
            entries[i].pte_PackageID = pkg;
            entries[i].pte_ClusterID = cluster;
            entries[i].pte_CoreID = core;
            entries[i].pte_ThreadID = thread;
            break;
        }
    }
}

/*
 * /cpus/cpu-map arranges the harts: socketN holding clusterN holding
 * coreN, optionally holding threadN, each leaf naming its hart with a
 * "cpu" phandle. Nodes come out of the walk depth first, so counting
 * the names as they pass reproduces the hierarchy; the subtree ends at
 * the first node named none of these.
 */
static void ParseCPUMap(struct ProcessorBase *ProcessorBase,
                        const fdt_node_t *cpunodes, ULONG ncpu)
{
    struct ProcessorTopology *topo = ProcessorBase->Topology;
    struct ProcessorTopologyEntry *entries;
    fdt_node_t node, cpumap;
    ULONG pkgs = 0, clusters = 0, cores = 0;
    ULONG core_in_pkg = 0, threads = 0, maxthreads = 1;

    if (!topo)
        return;
    entries = (struct ProcessorTopologyEntry *)topo->pt_Entries;

    cpumap = FindByName("cpu-map");
    if (cpumap == FDT_NONE)
        return;

    for (node = FDT_NextNode(cpumap); node != FDT_NONE;
         node = FDT_NextNode(node))
    {
        CONST_STRPTR name = FDT_NodeName(node);
        ULONG ph;

        if (!name)
            break;
        if (!strncmp(name, "socket", 6))
        {
            pkgs++;
            core_in_pkg = 0;
        }
        else if (!strncmp(name, "cluster", 7))
            clusters++;
        else if (!strncmp(name, "core", 4))
        {
            cores++;
            threads = 0;
            ph = FDT_GetPropU32(node, "cpu", 0);
            if (ph)
                AssignTopology(entries, cpunodes, ncpu, ph,
                               pkgs ? pkgs - 1 : 0,
                               clusters ? clusters - 1 : 0,
                               core_in_pkg, 0);
            core_in_pkg++;
        }
        else if (!strncmp(name, "thread", 6))
        {
            ph = FDT_GetPropU32(node, "cpu", 0);
            if (ph)
                AssignTopology(entries, cpunodes, ncpu, ph,
                               pkgs ? pkgs - 1 : 0,
                               clusters ? clusters - 1 : 0,
                               core_in_pkg ? core_in_pkg - 1 : 0, threads);
            threads++;
            if (threads > maxthreads)
                maxthreads = threads;
        }
        else
            break;
    }

    topo->pt_Packages = pkgs ? pkgs : 1;
    topo->pt_Clusters = clusters ? clusters : 1;
    topo->pt_Cores = cores ? cores : topo->pt_Count;
    topo->pt_ThreadsPerCore = maxthreads;
}

static LONG Processor_Init(struct ProcessorBase *ProcessorBase)
{
    struct RiscVProcessorInformation *info;
    struct ProcessorTopology *topo = ProcessorBase->Topology;
    fdt_node_t *cpunodes;
    struct TagItem *bootinfo;
    APTR dtb = NULL;
    UQUAD vendor = 0, archid = 0, impl = 0;
    ULONG i, found = 0;

    D(bug("[processor.riscv] %s: %u hart(s)\n", __func__,
          ProcessorBase->cpucount));

    info = AllocVec(ProcessorBase->cpucount * sizeof(*info),
                    MEMF_ANY | MEMF_CLEAR);
    if (!info)
        return FALSE;
    ProcessorBase->Private1 = info;

    Processor_PlatformReadIDs(&vendor, &archid, &impl);

    for (i = 0; i < ProcessorBase->cpucount; i++)
    {
        info[i].HartID = i;
#if __riscv_xlen == 64
        info[i].Family = CPUFAMILY_RISCV_RV64;
#else
        info[i].Family = CPUFAMILY_RISCV_RV32;
#endif
        info[i].VendorID = vendor;
        info[i].ArchID = archid;
        info[i].ImplID = impl;
    }

    cpunodes = AllocVec(ProcessorBase->cpucount * sizeof(fdt_node_t),
                        MEMF_ANY);

    bootinfo = KrnGetBootInfo();
    if (bootinfo)
        dtb = (APTR)GetTagData(KRN_FlattenedDeviceTree, 0, bootinfo);

    if (cpunodes && dtb && FDT_Open(dtb))
    {
        fdt_node_t cpus = FindByName("cpus");
        fdt_node_t node;

        if (cpus != FDT_NONE)
        {
            for (node = FDT_NextNode(cpus);
                 node != FDT_NONE && found < ProcessorBase->cpucount;
                 node = FDT_NextNode(node))
            {
                CONST_STRPTR name = FDT_NodeName(node);

                if (name && !strcmp(name, "cpu") && FDT_HasProp(node, "reg"))
                {
                    cpunodes[found] = node;
                    ParseCPUNode(&info[found], node, found);
                    found++;
                }
            }
        }

        ParseCPUMap(ProcessorBase, cpunodes, found);
    }

    FreeVec(cpunodes);

    /* The hart ids are the physical ids, whether or not a cpu-map
       refined the rest */
    if (topo)
    {
        struct ProcessorTopologyEntry *entries =
            (struct ProcessorTopologyEntry *)topo->pt_Entries;

        for (i = 0; i < topo->pt_Count; i++)
            entries[i].pte_PhysicalID = info[i].HartID;
    }

    D(
        for (i = 0; i < ProcessorBase->cpucount; i++)
        {
            bug("[processor.riscv] hart %u: id %u isa '%s' features 0x%08x\n",
                i, info[i].HartID,
                info[i].ISAString ? info[i].ISAString : "(none)",
                info[i].Features);
        }
    )

    return TRUE;
}

ADD2INITLIB(Processor_Init, 1)
