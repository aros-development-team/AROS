/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Reading the flattened device tree the firmware handed over.

    The blob is walked in place on every call rather than being turned
    into a tree up front: drivers look things up a handful of times at
    init and never again, so the walk costs less than the memory a
    parsed copy would need - and it works before there is an allocator.
*/

#include <aros/macros.h>
#include <string.h>

#include "include/fdt.h"

#define FDT_MAGIC       0xd00dfeed

#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

struct fdt_header
{
    ULONG magic;
    ULONG totalsize;
    ULONG off_dt_struct;
    ULONG off_dt_strings;
    ULONG off_mem_rsvmap;
    ULONG version;
    ULONG last_comp_version;
    ULONG boot_cpuid_phys;
    ULONG size_dt_strings;
    ULONG size_dt_struct;
};

static const UBYTE *fdt_base;
static const ULONG *fdt_struct;
static const char  *fdt_strings;
static ULONG        fdt_structlen;

#define be32(x) AROS_BE2LONG(x)

/* Offsets are in cells from the start of the structure block */
#define CELL(off)   fdt_struct[(off)]

BOOL FDT_Open(const void *dtb)
{
    const struct fdt_header *h = dtb;

    fdt_base = NULL;

    if (!dtb || be32(h->magic) != FDT_MAGIC)
        return FALSE;

    fdt_base      = dtb;
    fdt_struct    = (const ULONG *)(fdt_base + be32(h->off_dt_struct));
    fdt_strings   = (const char *)(fdt_base + be32(h->off_dt_strings));
    fdt_structlen = be32(h->size_dt_struct) / 4;

    return TRUE;
}

BOOL FDT_IsOpen(void)
{
    return fdt_base ? TRUE : FALSE;
}

/* Cells taken by a node name, including its terminator, rounded up */
static ULONG name_cells(LONG off)
{
    const char *name = (const char *)&CELL(off);
    ULONG len = strlen(name);

    return (len + 4) / 4;
}

/* Step from a token to the next one */
static LONG next_token(LONG off)
{
    switch (be32(CELL(off)))
    {
    case FDT_BEGIN_NODE:
        return off + 1 + name_cells(off + 1);

    case FDT_PROP:
    {
        ULONG len = be32(CELL(off + 1));

        return off + 3 + (len + 3) / 4;
    }

    case FDT_END_NODE:
    case FDT_NOP:
        return off + 1;

    default:
        return -1;
    }
}

fdt_node_t FDT_Root(void)
{
    LONG off = 0;

    if (!fdt_base)
        return FDT_NONE;

    /* Skip any leading NOPs */
    while (off < (LONG)fdt_structlen && be32(CELL(off)) == FDT_NOP)
        off++;

    return (be32(CELL(off)) == FDT_BEGIN_NODE) ? off : FDT_NONE;
}

fdt_node_t FDT_NextNode(fdt_node_t node)
{
    LONG off;

    if (!fdt_base || node == FDT_NONE)
        return FDT_NONE;

    for (off = next_token(node); off >= 0 && off < (LONG)fdt_structlen;
         off = next_token(off))
    {
        ULONG tok = be32(CELL(off));

        if (tok == FDT_BEGIN_NODE)
            return off;
        if (tok == FDT_END)
            break;
    }

    return FDT_NONE;
}

CONST_STRPTR FDT_NodeName(fdt_node_t node)
{
    static char buf[64];
    const char *name;
    ULONG i;

    if (!fdt_base || node == FDT_NONE ||
        be32(CELL(node)) != FDT_BEGIN_NODE)
        return NULL;

    name = (const char *)&CELL(node + 1);

    /* Drop the unit address - callers want "serial", not
       "serial@20300000" */
    for (i = 0; i < sizeof(buf) - 1 && name[i] && name[i] != '@'; i++)
        buf[i] = name[i];
    buf[i] = 0;

    return buf;
}

/*
 * Properties belong to the node until its first child or its end, so
 * scanning stops at the first nested FDT_BEGIN_NODE.
 */
CONST_APTR FDT_GetProp(fdt_node_t node, CONST_STRPTR name, ULONG *len)
{
    LONG off;

    if (!fdt_base || node == FDT_NONE || !name)
        return NULL;

    for (off = next_token(node); off >= 0 && off < (LONG)fdt_structlen;
         off = next_token(off))
    {
        ULONG tok = be32(CELL(off));

        if (tok == FDT_PROP)
        {
            ULONG plen = be32(CELL(off + 1));
            const char *pname = fdt_strings + be32(CELL(off + 2));

            if (!strcmp(pname, name))
            {
                if (len)
                    *len = plen;
                return (CONST_APTR)&CELL(off + 3);
            }
            continue;
        }

        /* A child node, the end of this one, or the end of the tree */
        if (tok != FDT_NOP)
            break;
    }

    return NULL;
}

BOOL FDT_HasProp(fdt_node_t node, CONST_STRPTR name)
{
    return FDT_GetProp(node, name, NULL) ? TRUE : FALSE;
}

ULONG FDT_GetPropU32(fdt_node_t node, CONST_STRPTR name, ULONG def)
{
    ULONG len = 0;
    const ULONG *p = FDT_GetProp(node, name, &len);

    return (p && len >= 4) ? be32(*p) : def;
}

CONST_STRPTR FDT_GetPropStr(fdt_node_t node, CONST_STRPTR name)
{
    ULONG len = 0;

    return (CONST_STRPTR)FDT_GetProp(node, name, &len);
}

BOOL FDT_IsCompatible(fdt_node_t node, CONST_STRPTR compat)
{
    ULONG len = 0, o = 0;
    const char *list = FDT_GetProp(node, "compatible", &len);

    if (!list || !compat)
        return FALSE;

    while (o < len)
    {
        if (!strcmp(&list[o], compat))
            return TRUE;
        o += strlen(&list[o]) + 1;
    }

    return FALSE;
}

fdt_node_t FDT_FindCompatible(fdt_node_t from, CONST_STRPTR compat)
{
    fdt_node_t n = (from == FDT_NONE) ? FDT_Root() : FDT_NextNode(from);

    for (; n != FDT_NONE; n = FDT_NextNode(n))
    {
        if (FDT_IsCompatible(n, compat))
            return n;
    }

    return FDT_NONE;
}

UQUAD FDT_ReadCells(const ULONG *cells, ULONG count)
{
    UQUAD v = 0;
    ULONG i;

    for (i = 0; i < count; i++)
        v = (v << 32) | be32(cells[i]);

    return v;
}

/*
 * The cell counts for a node's own reg come from its parent, so the
 * tree has to be re-walked to find who that is. Depth is tracked by
 * counting BEGIN/END tokens from the root.
 */
static fdt_node_t fdt_parent(fdt_node_t node)
{
    LONG off, depth = 0;
    fdt_node_t stack[16];

    if (!fdt_base || node == FDT_NONE || node == FDT_Root())
        return FDT_NONE;

    for (off = FDT_Root(); off >= 0 && off < (LONG)fdt_structlen;
         off = next_token(off))
    {
        ULONG tok = be32(CELL(off));

        if (tok == FDT_BEGIN_NODE)
        {
            if (off == node)
                return (depth > 0) ? stack[depth - 1] : FDT_NONE;
            if (depth < (LONG)(sizeof(stack) / sizeof(stack[0])))
                stack[depth] = off;
            depth++;
        }
        else if (tok == FDT_END_NODE)
        {
            if (--depth < 0)
                break;
        }
        else if (tok == FDT_END)
            break;
    }

    return FDT_NONE;
}

ULONG FDT_ParentAddressCells(fdt_node_t node)
{
    fdt_node_t p = fdt_parent(node);

    /* The specification's default when nobody says otherwise */
    return (p == FDT_NONE) ? 2 : FDT_GetPropU32(p, "#address-cells", 2);
}

ULONG FDT_ParentSizeCells(fdt_node_t node)
{
    fdt_node_t p = fdt_parent(node);

    return (p == FDT_NONE) ? 1 : FDT_GetPropU32(p, "#size-cells", 1);
}

BOOL FDT_GetReg(fdt_node_t node, ULONG index, UQUAD *addr, UQUAD *size)
{
    ULONG len = 0, ac, sc, stride;
    const ULONG *reg = FDT_GetProp(node, "reg", &len);

    if (!reg)
        return FALSE;

    ac = FDT_ParentAddressCells(node);
    sc = FDT_ParentSizeCells(node);
    stride = ac + sc;

    if (!stride || (index + 1) * stride * 4 > len)
        return FALSE;

    if (addr)
        *addr = FDT_ReadCells(&reg[index * stride], ac);
    if (size)
        *size = sc ? FDT_ReadCells(&reg[index * stride + ac], sc) : 0;

    return TRUE;
}

BOOL FDT_GetRegNamed(fdt_node_t node, CONST_STRPTR name, UQUAD *addr,
                     UQUAD *size)
{
    ULONG len = 0, o = 0, index = 0;
    const char *names = FDT_GetProp(node, "reg-names", &len);

    if (!names || !name)
        return FALSE;

    while (o < len)
    {
        if (!strcmp(&names[o], name))
            return FDT_GetReg(node, index, addr, size);
        o += strlen(&names[o]) + 1;
        index++;
    }

    return FALSE;
}
