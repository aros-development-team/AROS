/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    m68kemu.library — full containment memory model
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/bptr.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <aros/debug.h>
#include <string.h>
#include <stdio.h>

#include "m68kemu_intern.h"
#include "m68kemu_offsets.h"
#include "m68kemu_thunks.h"

struct M68KEmuContext *M68KEmu_CreateContext(struct M68KEmuLibBase *base)
{
    UBYTE *mem;
    struct M68KEmuContext *ctx;
    ULONG heap_size;

    mem = (UBYTE *)AllocMem(M68KEMU_MEM_SIZE, MEMF_CLEAR | MEMF_31BIT);
    if (!mem)
        return NULL;

    ctx = (struct M68KEmuContext *)AllocMem(sizeof(struct M68KEmuContext), MEMF_CLEAR);
    if (!ctx)
    {
        FreeMem(mem, M68KEMU_MEM_SIZE);
        return NULL;
    }

    ctx->mem      = mem;
    ctx->mem_size = M68KEMU_MEM_SIZE;
    ctx->emuBase  = base;

    ctx->stack_top    = M68KEMU_MEM_SIZE;
    ctx->stack_bottom = M68KEMU_MEM_SIZE - M68KEMU_STACK_SIZE;

    ctx->heap_start = M68KEMU_HEAP_BASE;
    ctx->heap_end   = ctx->stack_bottom;
    heap_size       = ctx->heap_end - ctx->heap_start;

    /* One big free block */
    ctx->heap_free = ctx->heap_start;
    m68k_write32(ctx, ctx->heap_start,     heap_size);
    m68k_write32(ctx, ctx->heap_start + 4, 0);

    return ctx;
}

void M68KEmu_DestroyContext(struct M68KEmuContext *ctx)
{
    if (!ctx) return;
    if (ctx->handles) FreeVec(ctx->handles);
    if (ctx->mem) FreeMem(ctx->mem, ctx->mem_size);
    FreeMem(ctx, sizeof(struct M68KEmuContext));
}

ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx, ULONG size, ULONG requirements)
{
    ULONG needed, prev, cur;

    size   = (size + 3) & ~3;
    needed = size + M68KEMU_HEAP_HDR;  /* header */

    prev = 0;
    cur  = ctx->heap_free;

    while (cur)
    {
        ULONG blk_size = m68k_read32(ctx, cur);
        ULONG blk_next = m68k_read32(ctx, cur + 4);

        if (blk_size >= needed)
        {
            ULONG remainder = blk_size - needed;

            if (remainder >= M68KEMU_HEAP_MIN_SPLIT)
            {
                /* Split: keep remainder as free, allocate from end */
                m68k_write32(ctx, cur, remainder);
                m68k_write32(ctx, cur + remainder, needed);
                return cur + remainder + M68KEMU_HEAP_HDR;
            }
            else
            {
                /* Use whole block */
                needed = blk_size;
                if (prev)
                    m68k_write32(ctx, prev + 4, blk_next);
                else
                    ctx->heap_free = blk_next;
                m68k_write32(ctx, cur, needed);
                return cur + M68KEMU_HEAP_HDR;
            }
        }
        prev = cur;
        cur  = blk_next;
    }
    return 0;
}

void M68KEmu_HeapFree(struct M68KEmuContext *ctx, ULONG addr, ULONG size)
{
    ULONG blk, blk_size, prev, cur;

    if (!addr) return;

    blk      = addr - M68KEMU_HEAP_HDR;
    blk_size = m68k_read32(ctx, blk);

    /* Insert in address order */
    prev = 0;
    cur  = ctx->heap_free;
    while (cur && cur < blk)
    {
        prev = cur;
        cur  = m68k_read32(ctx, cur + 4);
    }

    m68k_write32(ctx, blk,     blk_size);
    m68k_write32(ctx, blk + 4, cur);

    if (prev)
        m68k_write32(ctx, prev + 4, blk);
    else
        ctx->heap_free = blk;

    /* Coalesce forward */
    if (cur && (blk + blk_size) == cur)
    {
        ULONG cur_size = m68k_read32(ctx, cur);
        m68k_write32(ctx, blk,     blk_size + cur_size);
        m68k_write32(ctx, blk + 4, m68k_read32(ctx, cur + 4));
        blk_size += cur_size;
    }

    /* Coalesce backward */
    if (prev)
    {
        ULONG prev_size = m68k_read32(ctx, prev);
        if ((prev + prev_size) == blk)
        {
            m68k_write32(ctx, prev,     prev_size + blk_size);
            m68k_write32(ctx, prev + 4, m68k_read32(ctx, blk + 4));
        }
    }
}

/* Copy a seglist LoadSeg() built from a hunk file into containment. dos
   relocated the segments to their host addresses; the relocation list it
   kept (GSLI_68KRELOCS) is applied again for the containment addresses. */
ULONG M68KEmu_LoadHunks(struct M68KEmuContext *ctx, BPTR segList)
{
    BPTR seg;
    ULONG entry_point = 0;
    ULONG nseg = 0, i;
    UBYTE **host;
    ULONG *cont, *size;
    struct HunkRelocTable *relocs = NULL;
    struct TagItem tags[] =
    {
        { GSLI_68KRELOCS, (IPTR)&relocs },
        { TAG_DONE,       0             }
    };

    for (seg = segList; seg; seg = *((BPTR *)BADDR(seg)))
        nseg++;
    if (!nseg)
        return 0;

    host = AllocVec(nseg * (sizeof(*host) + sizeof(*cont) + sizeof(*size)), MEMF_CLEAR);
    if (!host)
        return 0;
    cont = (ULONG *)(host + nseg);
    size = cont + nseg;

    for (seg = segList, i = 0; seg; seg = *((BPTR *)BADDR(seg)), i++)
    {
        ULONG *baddr     = (ULONG *)BADDR(seg);
        ULONG alloc_size = *(baddr - 1);              /* ULONG size + BPTR next + data */
        ULONG data_size  = alloc_size - sizeof(ULONG) - sizeof(BPTR);
        UBYTE *src       = (UBYTE *)baddr + sizeof(BPTR);
        ULONG m68k_addr;

        m68k_addr = M68KEmu_HeapAlloc(ctx, data_size, 0);
        if (!m68k_addr)
        {
            FreeVec(host);
            return 0;
        }

        CopyMem(src, m68k_to_host(ctx, m68k_addr), data_size);
        host[i] = src;
        cont[i] = m68k_addr;
        size[i] = data_size;

        if (!entry_point)
        {
            entry_point      = m68k_addr;
            ctx->entry_point = m68k_addr;
            ctx->seg_start   = m68k_addr;
        }
    }

    GetSegListInfo(segList, tags);
    if (relocs)
    {
        if (relocs->hrt_Count == 0xFFFFFFFFUL)
        {
            bug("[m68kemu] relocation list of seglist %p was lost, cannot run it\n", BADDR(segList));
            FreeVec(host);
            return 0;
        }
        for (i = 0; i < relocs->hrt_Count; i++)
        {
            const struct HunkReloc32 *r = &relocs->hrt_Relocs[i];
            UBYTE *p;
            ULONG val, original;

            if (r->hr_Hunk >= nseg || r->hr_Target >= nseg || r->hr_Offset + 4 > size[r->hr_Hunk])
            {
                bug("[m68kemu] bad relocation %lu (hunk %lu offset %lu target %lu)\n",
                    (unsigned long)i, (unsigned long)r->hr_Hunk, (unsigned long)r->hr_Offset,
                    (unsigned long)r->hr_Target);
                FreeVec(host);
                return 0;
            }
            p   = ctx->mem + cont[r->hr_Hunk] + r->hr_Offset;
            val = AROS_BE2LONG(*(ULONG *)p);
            if (r->hr_Type == HUNKRELOC_REL)
            {
                original = val + (ULONG)(IPTR)(host[r->hr_Hunk] + r->hr_Offset) - (ULONG)(IPTR)host[r->hr_Target];
                val = cont[r->hr_Target] + original - (cont[r->hr_Hunk] + r->hr_Offset);
            }
            else
            {
                original = val - (ULONG)(IPTR)host[r->hr_Target];
                val = cont[r->hr_Target] + original;
            }
            *(ULONG *)p = AROS_LONG2BE(val);
        }
    }

    FreeVec(host);
    return entry_point;
}

ULONG M68KEmu_SetupLibBase(struct M68KEmuContext *ctx, UWORD lib_id,
                            const char *name, UWORD num_vectors,
                            const struct M68KThunkEntry *thunks, ULONG num_thunks,
                            const struct M68KThunkEntry *gen_thunks, ULONG num_gen_thunks)
{
    ULONG vectors_size, total_size, region_offset, base_addr;
    UWORD i;

    if (ctx->num_libs >= M68KEMU_MAX_LIBS)
        return 0;

    vectors_size = (ULONG)num_vectors * M68K_JT_SLOT_SIZE;
    total_size   = vectors_size + ((lib_id == 0) ? M68K_SIZEOF_EXECBASE : M68K_SIZEOF_LIBRARY);

    /* Find next free spot in LIBBASE_REGION */
    region_offset = M68KEMU_LIBBASE_REGION;
    for (i = 0; i < ctx->num_libs; i++)
    {
        ULONG lib_end = ctx->libs[i].m68k_addr + ctx->libs[i].pos_size;
        if (lib_end > region_offset)
            region_offset = (lib_end + 3) & ~3;
    }

    if (region_offset + total_size > M68KEMU_LIBBASE_REGION + M68KEMU_LIBBASE_SIZE)
        return 0;

    base_addr = region_offset + vectors_size;

    /* Fill ALL jump vector slots with A-line trap + NOP + NOP */
    for (i = 0; i < num_vectors; i++)
    {
        ULONG slot_addr = base_addr - ((ULONG)(i + 1) * M68K_JT_SLOT_SIZE);
        m68k_write16(ctx, slot_addr,     M68K_OP_ALINE);
        m68k_write16(ctx, slot_addr + 2, M68K_OP_NOP);
        m68k_write16(ctx, slot_addr + 4, M68K_OP_NOP);
    }

    /* A program reads the base as a struct Library: AROS startup code checks
       lib_Version against the version it was built for. Report the native
       library's numbers. */
    {
        struct Library *native = name ? OpenLibrary((CONST_STRPTR)name, 0) : NULL;
        UWORD ver = native ? native->lib_Version : 0;
        UWORD rev = native ? native->lib_Revision : 0;
        if (native)
            CloseLibrary(native);
        m68k_write16(ctx, base_addr + 8, (UWORD)NT_LIBRARY << 8);     /* ln_Type, ln_Pri */
        m68k_write16(ctx, base_addr + 16, (UWORD)vectors_size);       /* lib_NegSize */
        m68k_write16(ctx, base_addr + 18, (lib_id == 0) ? M68K_SIZEOF_EXECBASE : M68K_SIZEOF_LIBRARY);
        m68k_write16(ctx, base_addr + M68K_LIB_VERSION, ver);
        m68k_write16(ctx, base_addr + M68K_LIB_REVISION, rev);
    }

    ctx->libs[ctx->num_libs].m68k_addr   = base_addr;
    ctx->libs[ctx->num_libs].pos_size    = (lib_id == 0) ? M68K_SIZEOF_EXECBASE : M68K_SIZEOF_LIBRARY;
    ctx->libs[ctx->num_libs].jt_start    = base_addr - vectors_size;
    ctx->libs[ctx->num_libs].lib_id      = lib_id;
    ctx->libs[ctx->num_libs].num_vectors = num_vectors;
    ctx->libs[ctx->num_libs].thunks      = thunks;
    ctx->libs[ctx->num_libs].num_thunks  = num_thunks;
    ctx->libs[ctx->num_libs].gen_thunks   = gen_thunks;
    ctx->libs[ctx->num_libs].num_gen_thunks = num_gen_thunks;
    if (name)
    {
        snprintf(ctx->libs[ctx->num_libs].name, sizeof(ctx->libs[ctx->num_libs].name), "%s", name);
    }
    ctx->num_libs++;

    /* Register in libmap for OpenLibrary lookups */
    if (name && ctx->num_libmap < M68KEMU_MAX_LIBMAP)
    {
        snprintf(ctx->libmap[ctx->num_libmap].name, sizeof(ctx->libmap[ctx->num_libmap].name), "%s", name);
        ctx->libmap[ctx->num_libmap].m68k_base = base_addr;
        ctx->num_libmap++;
    }

    return base_addr;
}

ULONG M68KEmu_FindLibBase(struct M68KEmuContext *ctx, const char *name)
{
    UWORD i;
    if (!name) return 0;
    for (i = 0; i < ctx->num_libmap; i++)
    {
        if (strcmp(ctx->libmap[i].name, name) == 0)
            return ctx->libmap[i].m68k_base;
    }
    return 0;
}

/* ── Hunk file loader (from raw memory buffer) ────────────────────── */

/* Hunk constants now in m68kemu_offsets.h */

static ULONG rd32(UBYTE **p)
{
    ULONG v = ((ULONG)(*p)[0] << 24) | ((ULONG)(*p)[1] << 16) |
              ((ULONG)(*p)[2] << 8)  |  (ULONG)(*p)[3];
    *p += 4;
    return v;
}

ULONG M68KEmu_LoadHunksFromMemory(struct M68KEmuContext *ctx, UBYTE *data, LONG size)
{
    UBYTE *p = data;
    UBYTE *end = data + size;
    ULONG magic, num_hunks, first, last;
    ULONG *hunk_sizes = NULL;
    ULONG *hunk_addrs = NULL;
    ULONG i, entry_point = 0;

    if (p + 4 > end) return 0;
    magic = rd32(&p);
    if (magic != HUNK_HEADER) return 0;

    /* Skip resident library names */
    while (p + 4 <= end)
    {
        ULONG n = rd32(&p);
        if (n == 0) break;
        p += n * 4;
    }

    if (p + 12 > end) return 0;
    num_hunks = rd32(&p);
    first     = rd32(&p);  /* unused but must be consumed */
    last      = rd32(&p);  /* unused but must be consumed */
    (void)first; (void)last;

    if (num_hunks == 0 || num_hunks > 100) return 0;

    /* Read hunk sizes (in longwords) */
    hunk_sizes = (ULONG *)AllocMem(num_hunks * sizeof(ULONG), MEMF_CLEAR);
    hunk_addrs = (ULONG *)AllocMem(num_hunks * sizeof(ULONG), MEMF_CLEAR);
    if (!hunk_sizes || !hunk_addrs) goto fail;

    for (i = 0; i < num_hunks; i++)
    {
        if (p + 4 > end) goto fail;
        hunk_sizes[i] = (rd32(&p) & HUNK_TYPE_MASK) * 4; /* mask off MEMF flags */
    }

    /* Allocate space for each hunk in containment memory */
    for (i = 0; i < num_hunks; i++)
    {
        ULONG addr = M68KEmu_HeapAlloc(ctx, hunk_sizes[i], 0);
        if (!addr) goto fail;
        hunk_addrs[i] = addr;
        if (i == 0) entry_point = addr;
    }

    /* Parse hunk contents */
    i = 0;
    while (p + 4 <= end)
    {
        ULONG type = rd32(&p) & HUNK_TYPE_MASK;

        if (type == HUNK_CODE || type == HUNK_DATA)
        {
            if (p + 4 > end) goto fail;
            ULONG longs = rd32(&p);
            ULONG bytes = longs * 4;
            if (p + bytes > end || i >= num_hunks) goto fail;

            /* Copy data into containment space */
            memcpy(m68k_to_host(ctx, hunk_addrs[i]), p, bytes);
            p += bytes;
        }
        else if (type == HUNK_BSS)
        {
            if (p + 4 > end) goto fail;
            rd32(&p); /* skip size, already zeroed */
        }
        else if (type == HUNK_RELOC32)
        {
            while (p + 4 <= end)
            {
                ULONG count = rd32(&p);
                if (count == 0) break;
                if (p + 4 > end) goto fail;
                ULONG ref_hunk = rd32(&p);
                if (ref_hunk >= num_hunks) goto fail;

                ULONG ref_base = hunk_addrs[ref_hunk];
                ULONG j;
                for (j = 0; j < count; j++)
                {
                    if (p + 4 > end) goto fail;
                    ULONG offset = rd32(&p);
                    if (i < num_hunks && offset + 3 < hunk_sizes[i])
                    {
                        ULONG addr = hunk_addrs[i] + offset;
                        ULONG val = m68k_read32(ctx, addr);
                        m68k_write32(ctx, addr, val + ref_base);
                    }
                }
            }
        }
        else if (type == HUNK_END)
        {
            i++;
        }
        else
        {
            /* Unknown hunk type — skip */
            break;
        }
    }

    ctx->entry_point = entry_point;
    ctx->seg_start   = entry_point;

    FreeMem(hunk_sizes, num_hunks * sizeof(ULONG));
    FreeMem(hunk_addrs, num_hunks * sizeof(ULONG));
    return entry_point;

fail:
    if (hunk_addrs) {
        for (i = 0; i < num_hunks; i++) {
            if (hunk_addrs[i])
                M68KEmu_HeapFree(ctx, hunk_addrs[i], hunk_sizes[i]);
        }
        FreeMem(hunk_addrs, num_hunks * sizeof(ULONG));
    }
    if (hunk_sizes) FreeMem(hunk_sizes, num_hunks * sizeof(ULONG));
    return 0;
}

/* Auto-open libraries: scan containment memory for the C startup pattern:
   N zero longwords (base pointers) followed immediately by N
   consecutive NUL-terminated ".library" name strings. */
void M68KEmu_AutoOpenLibs(struct M68KEmuContext *ctx)
{
    UBYTE *base = ctx->mem;
    ULONG sz = ctx->mem_size;
    ULONG off = M68KEMU_HEAP_BASE;

    while (off + 10 < sz)
    {
        if (base[off] != '.' || memcmp(base + off, ".library", 8) != 0 ||
            base[off + 8] != 0)
        {
            off++;
            continue;
        }

        /* Found ".library\0" — scan back for name start */
        ULONG name_start = off;
        while (name_start > 0 && base[name_start - 1] >= 0x20 &&
               base[name_start - 1] < 0x7f)
            name_start--;

        /* Collect consecutive library name strings */
        ULONG names[16];
        ULONG count = 0;
        ULONG p = name_start;
        while (p < sz && count < 16)
        {
            ULONG q = p;
            while (q < sz && base[q] >= 0x20 && base[q] < 0x7f) q++;
            if (q >= p + 9 && q < sz && base[q] == 0 &&
                memcmp(base + q - 8, ".library", 8) == 0)
            {
                names[count++] = p;
                p = q + 1;
            }
            else
                break;
        }

        if (count == 0) { off++; continue; }

        /* Check if count*4 bytes before first name are all zero */
        if (names[0] < count * 4 + 4) { off = p; continue; }
        ULONG table_start = names[0] - count * 4;

        BOOL all_zero = TRUE;
        ULONG j;
        for (j = 0; j < count * 4; j++)
            if (base[table_start + j] != 0) { all_zero = FALSE; break; }

        if (!all_zero) { off = p; continue; }

        /* Fill base pointer slots */
        for (j = 0; j < count; j++)
        {
            const char *lname = (const char *)(base + names[j]);
            ULONG fakebase = M68KEmu_OpenLibrary(ctx, lname);
            if (fakebase)
                m68k_write32(ctx, table_start + j * 4, fakebase);
        }

        off = p;
    }
}

/* ── Native BPTR handles ──────────────────────────────────────────────
   A native BPTR is a 64-bit pointer on this side and cannot travel through
   a 32-bit m68k register. The m68k side gets an index into a table instead;
   the same native BPTR always maps to the same handle. Values outside the
   handle range are m68k BPTRs into containment (BSTRs, in-memory seglists)
   and translate by address. */

ULONG m68k_bptr_out(struct M68KEmuContext *ctx, BPTR b)
{
    ULONG i, free_slot = (ULONG)-1;

    if (b == BNULL)
        return 0;
    if (!ctx->handles)
    {
        ctx->handles = AllocVec(M68KEMU_MAX_HANDLES * sizeof(BPTR), MEMF_CLEAR);
        if (!ctx->handles)
            return 0;
    }
    for (i = 0; i < ctx->num_handles; i++)
    {
        if (ctx->handles[i] == b)
            return M68KEMU_HANDLE_BASE + i * 4;
        if (ctx->handles[i] == BNULL && free_slot == (ULONG)-1)
            free_slot = i;
    }
    if (free_slot == (ULONG)-1)
    {
        if (ctx->num_handles >= M68KEMU_MAX_HANDLES)
        {
            bug("[m68kemu] out of BPTR handles\n");
            return 0;
        }
        free_slot = ctx->num_handles++;
    }
    ctx->handles[free_slot] = b;
    return M68KEMU_HANDLE_BASE + free_slot * 4;
}

BPTR m68k_bptr_in(struct M68KEmuContext *ctx, ULONG v)
{
    if (v == 0)
        return BNULL;
    if (v >= M68KEMU_HANDLE_BASE && v < M68KEMU_HANDLE_BASE + M68KEMU_MAX_HANDLES * 4)
    {
        ULONG i = (v - M68KEMU_HANDLE_BASE) / 4;
        if (ctx->handles && i < ctx->num_handles)
            return ctx->handles[i];
        return BNULL;
    }
    /* an m68k BPTR into containment */
    return MKBADDR(m68k_to_host(ctx, v << 2));
}

void m68k_bptr_drop(struct M68KEmuContext *ctx, ULONG v)
{
    if (v >= M68KEMU_HANDLE_BASE && v < M68KEMU_HANDLE_BASE + M68KEMU_MAX_HANDLES * 4)
    {
        ULONG i = (v - M68KEMU_HANDLE_BASE) / 4;
        if (ctx->handles && i < ctx->num_handles)
            ctx->handles[i] = BNULL;
    }
}
