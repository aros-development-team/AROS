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
    /* Which cpu@ node the hart we booted on is, so the right
       supervisor context can be picked out of the PLIC's list */
    int boot_cpu_index = 0;

    info->mem_base = 0;
    info->mem_size = 0;
    info->nregions = 0;
    info->bootargs = (void *)0;
    info->ncpus = 0;
    info->totalsize = 0;
    info->tb_freq = 0;
    info->initrd_start = 0;
    info->initrd_end = 0;
    info->plic_base = 0;
    info->plic_size = 0;
    info->plic_ndev = 0;
    info->plic_context = -1;
    info->plic_ncontexts = 0;

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
                     str_eq(pname, "reg"))
            {
                /* "reg" may list several base/size pairs, and there may
                   be more than one memory node */
                uint32_t pair = (addr_cells + size_cells) * 4;
                uint32_t off;

                for (off = 0; off + pair <= len &&
                              info->nregions < KRN_MAX_MEMREGIONS; off += pair)
                {
                    const uint32_t *c = val + off / 4;
                    uint64_t base = read_cells(c, addr_cells);
                    uint64_t size = read_cells(c + addr_cells, size_cells);

                    if (!size)
                        continue;

                    info->regions[info->nregions].base = base;
                    info->regions[info->nregions].size = size;
                    info->nregions++;

                    /* The largest bank is where the system heap goes */
                    if (size > info->mem_size)
                    {
                        info->mem_base = base;
                        info->mem_size = size;
                    }
                }
            }
            else if (depth == 3 && str_eq(node1, "cpus") &&
                     str_prefix(node2, "cpu@") && str_eq(pname, "reg") &&
                     len >= 4)
            {
                if (read_cells(val, 1) == __boot_hartid)
                    boot_cpu_index = (int)info->ncpus - 1;
            }
            else if (depth == 3 && str_prefix(node2, "plic@") &&
                     str_eq(pname, "reg"))
            {
                info->plic_base = read_cells(val, addr_cells);
                info->plic_size = read_cells(val + addr_cells, size_cells);
            }
            else if (depth == 3 && str_prefix(node2, "plic@") &&
                     str_eq(pname, "riscv,ndev"))
            {
                info->plic_ndev = be32(*val);
            }
            else if (depth == 3 && str_prefix(node2, "plic@") &&
                     str_eq(pname, "interrupts-extended"))
            {
                /*
                 * Pairs of (interrupt controller phandle, cause). Each
                 * pair is one PLIC context, numbered by its position.
                 * Cause 9 is a supervisor external interrupt - the one
                 * this kernel runs in - and the contexts appear in the
                 * same order as the cpu nodes.
                 */
                uint32_t npairs = len / 8, i, seen = 0;

                for (i = 0; i < npairs; i++)
                {
                    if (be32(val[i * 2 + 1]) != 9)
                        continue;

                    if (info->plic_ncontexts < KRN_MAX_PLIC_CONTEXTS)
                        info->plic_contexts[info->plic_ncontexts++] =
                            (int)i;

                    if ((int)seen == boot_cpu_index)
                        info->plic_context = (int)i;
                    seen++;
                }
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

/*
 * Dump the device tree: every node, with the properties that say what
 * the hardware is. Enabled with the "dumpdt" bootarg - the point is to
 * see what a board actually offers before writing drivers for it.
 */
/* Is this property a list of NUL-terminated strings? */
static int fdt_is_strlist(const char *v, uint32_t len)
{
    uint32_t i;

    /* Must end with a terminator, and hold no empty strings - without
       that test a cell holding zero looks like a list of them */
    if (len < 2 || v[len - 1] != 0 || v[0] == 0)
        return 0;
    for (i = 0; i < len - 1; i++)
    {
        if (v[i] == 0)
        {
            if (v[i + 1] == 0)
                return 0;
            continue;
        }
        if (v[i] < 0x20 || v[i] > 0x7e)
            return 0;
    }
    return 1;
}

static void fdt_indent(int depth)
{
    int i;

    for (i = 0; i < depth; i++)
        krnSBIPutStr("  ");
}

#define FDT_MAXDEPTH 24

/*
 * Print the whole tree. Everything is shown - bringing a device up
 * needs its interrupts, its clocks and the rest, not just its address,
 * and on this machine the device tree is the only description there is
 * where the firmware publishes no ACPI tables.
 */
void krnDumpFDT(void *dtb)
{
    const struct fdt_header *hdr = dtb;
    const uint32_t *p, *end;
    const char *strings;
    int depth = 0;
    /* #address-cells/#size-cells each node declares for its children */
    unsigned int ac[FDT_MAXDEPTH], sc[FDT_MAXDEPTH];

    if (!dtb || be32(hdr->magic) != FDT_MAGIC)
        return;

    strings = (const char *)dtb + be32(hdr->off_dt_strings);
    p = (const uint32_t *)((const char *)dtb + be32(hdr->off_dt_struct));
    end = (const uint32_t *)((const char *)dtb + be32(hdr->totalsize));

    ac[0] = 2;
    sc[0] = 1;

    krnSBIPutStr("--- device tree ---\n");

    while (p < end)
    {
        uint32_t token = be32(*p++);

        if (token == FDT_BEGIN_NODE)
        {
            const char *name = (const char *)p;
            uint32_t namelen = 0;

            while (name[namelen])
                namelen++;
            p += (namelen + 4) / 4;

            fdt_indent(depth);
            krnSBIPutStr(*name ? name : "/");
            krnSBIPutStr("\n");

            depth++;
            /* Until it says otherwise, a node inherits its parent's */
            if (depth < FDT_MAXDEPTH)
            {
                ac[depth] = ac[depth - 1];
                sc[depth] = sc[depth - 1];
            }
        }
        else if (token == FDT_END_NODE)
        {
            if (depth > 0)
                depth--;
        }
        else if (token == FDT_PROP)
        {
            uint32_t len = be32(*p++);
            uint32_t nameoff = be32(*p++);
            const char *pname = strings + nameoff;
            const char *val = (const char *)p;
            unsigned int d = (depth < FDT_MAXDEPTH) ? depth : FDT_MAXDEPTH - 1;

            p += (len + 3) / 4;

            /* Cell counts apply to this node's children */
            if (str_eq(pname, "#address-cells") && len == 4)
                ac[d] = (unsigned int)be32(*(const uint32_t *)val);
            else if (str_eq(pname, "#size-cells") && len == 4)
                sc[d] = (unsigned int)be32(*(const uint32_t *)val);

            fdt_indent(depth);
            krnSBIPutStr(".");
            krnSBIPutStr(pname);

            if (!len)
            {
                krnSBIPutStr("\n");
                continue;
            }

            krnSBIPutStr(" = ");

            if (fdt_is_strlist(val, len))
            {
                uint32_t o = 0;

                while (o < len)
                {
                    krnSBIPutStr(val + o);
                    while (o < len && val[o])
                        o++;
                    o++;
                    if (o < len)
                        krnSBIPutStr(", ");
                }
            }
            else if (str_eq(pname, "reg") && (len & 3) == 0 &&
                     (ac[d - 1] + sc[d - 1]) &&
                     (len / 4) % (ac[d - 1] + sc[d - 1]) == 0)
            {
                /* Address/size pairs, in the parent's cell counts */
                unsigned int pac = ac[d - 1], psc = sc[d - 1];
                unsigned int nent = (len / 4) / (pac + psc), e;
                const uint32_t *c = (const uint32_t *)val;

                for (e = 0; e < nent; e++)
                {
                    if (e)
                        krnSBIPutStr(", ");
                    krnSBIPutHex(read_cells(c, pac));
                    c += pac;
                    if (psc)
                    {
                        krnSBIPutStr(" size ");
                        krnSBIPutHex(read_cells(c, psc));
                        c += psc;
                    }
                }
            }
            else if ((len & 3) == 0)
            {
                /* Anything else that is a list of cells - interrupts,
                   ranges, phandles, clock specifiers */
                uint32_t n = len / 4, i;

                for (i = 0; i < n; i++)
                {
                    if (i)
                        krnSBIPutStr(i % 8 ? " " : "\n");
                    if (i && !(i % 8))
                    {
                        fdt_indent(depth + 1);
                        krnSBIPutStr("  ");
                    }
                    krnSBIPutHex32(be32(((const uint32_t *)val)[i]));
                }
            }
            else
            {
                uint32_t i;

                for (i = 0; i < len; i++)
                    krnSBIPutHex8((unsigned char)val[i]);
            }
            krnSBIPutStr("\n");
        }
        else if (token == FDT_END)
            break;
        else if (token != FDT_NOP)
            break;
    }
    krnSBIPutStr("--- end device tree ---\n");
}
