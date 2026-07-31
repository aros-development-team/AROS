/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: Minimal flattened device tree parser for early boot.

    Extracts just what the early kernel needs from the DTB handed over
    by OpenSBI: the first /memory range, /chosen bootargs and the cpu
    count. The full tree is passed on via KRN_FlattenedDeviceTree for
    the kernel proper / drivers to interpret.
*/

#include <inttypes.h>

#include "kernel_intern.h"

#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

struct fdt_header
{
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

static inline uint32_t be32(uint32_t v)
{
    return __builtin_bswap32(v);
}

static int str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int str_prefix(const char *s, const char *prefix)
{
    while (*prefix && *s == *prefix) { s++; prefix++; }
    return *prefix == '\0';
}

static uint64_t read_cells(const uint32_t *p, uint32_t cells)
{
    uint64_t v = 0;
    while (cells--)
        v = (v << 32) | be32(*p++);
    return v;
}

int krnParseFDT(void *dtb, struct krnFDTInfo *info)
{
    const struct fdt_header *hdr = dtb;
    const uint32_t *p, *end;
    const char *strings;
    int depth = 0;
    /* The node names at depth 1/2 currently being walked */
    const char *node1 = "", *node2 = "";
    uint32_t addr_cells = 2, size_cells = 2;

    info->mem_base = 0;
    info->mem_size = 0;
    info->bootargs = (void *)0;
    info->ncpus = 0;
    info->totalsize = 0;
    info->tb_freq = 0;
    info->initrd_start = 0;
    info->initrd_end = 0;

    if (!dtb || be32(hdr->magic) != FDT_MAGIC)
        return 0;

    info->totalsize = be32(hdr->totalsize);
    strings = (const char *)dtb + be32(hdr->off_dt_strings);
    p = (const uint32_t *)((const char *)dtb + be32(hdr->off_dt_struct));
    end = (const uint32_t *)((const char *)dtb + info->totalsize);

    while (p < end)
    {
        uint32_t token = be32(*p++);

        switch (token)
        {
        case FDT_BEGIN_NODE:
        {
            const char *name = (const char *)p;
            uint32_t namelen = 0;

            while (name[namelen])
                namelen++;
            p += (namelen + 4) / 4;     /* name + NUL, padded to 4 */

            depth++;
            if (depth == 2)
                node1 = name;
            else if (depth == 3)
                node2 = name;

            if (depth == 3 && str_eq(node1, "cpus") && str_prefix(node2, "cpu@"))
                info->ncpus++;
            break;
        }

        case FDT_END_NODE:
            depth--;
            break;

        case FDT_PROP:
        {
            uint32_t len = be32(*p++);
            uint32_t nameoff = be32(*p++);
            const char *pname = strings + nameoff;
            const uint32_t *val = p;

            p += (len + 3) / 4;

            if (depth == 1)
            {
                /* Root properties: cell sizes for the level-1 'reg's */
                if (str_eq(pname, "#address-cells"))
                    addr_cells = be32(*val);
                else if (str_eq(pname, "#size-cells"))
                    size_cells = be32(*val);
            }
            else if (depth == 2 && str_prefix(node1, "memory") &&
                     str_eq(pname, "reg") && info->mem_size == 0 &&
                     len >= (addr_cells + size_cells) * 4)
            {
                info->mem_base = read_cells(val, addr_cells);
                info->mem_size = read_cells(val + addr_cells, size_cells);
            }
            else if (depth == 2 && str_eq(node1, "chosen") &&
                     str_eq(pname, "bootargs") && len > 0)
            {
                info->bootargs = (const char *)val;
            }
            else if (depth == 2 && str_eq(node1, "cpus") &&
                     str_eq(pname, "timebase-frequency") && len >= 4)
            {
                info->tb_freq = be32(*val);
            }
            else if (depth == 2 && str_eq(node1, "chosen") &&
                     str_eq(pname, "linux,initrd-start"))
            {
                info->initrd_start = (len >= 8) ? read_cells(val, 2)
                                                : be32(*val);
            }
            else if (depth == 2 && str_eq(node1, "chosen") &&
                     str_eq(pname, "linux,initrd-end"))
            {
                info->initrd_end = (len >= 8) ? read_cells(val, 2)
                                              : be32(*val);
            }
            break;
        }

        case FDT_NOP:
            break;

        case FDT_END:
        default:
            return info->mem_size != 0;
        }
    }

    return info->mem_size != 0;
}
