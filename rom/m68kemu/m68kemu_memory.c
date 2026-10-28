/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    m68kemu.library — full containment memory model
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/notify.h>
#include <dos/dosasl.h>
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
#include "m68kemu_shadow.h"
extern ULONG shadow_register(struct M68KEmuContext *ctx, ULONG m68k_addr, void *native, UWORD type);

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
    /* programs size windows from the classic border widths; M68KEMU_CLASSICBORDERS=0 shows the real ones */
    {
        char v[4];
        ctx->classic_borders = !(GetVar((CONST_STRPTR)"M68KEMU_CLASSICBORDERS", (STRPTR)v, sizeof(v), 0) > 0 && v[0] == '0');
    }
    ctx->emuBase  = base;
    ctx->cacr     = CACRF_EnableI | CACRF_EnableD | CACRF_CopyBack;

    ctx->stack_top    = M68KEMU_MEM_SIZE;
    ctx->stack_bottom = M68KEMU_MEM_SIZE - M68KEMU_STACK_SIZE;

    ctx->heap_start = M68KEMU_HEAP_BASE;
    ctx->heap_end   = ctx->stack_bottom;
    heap_size       = ctx->heap_end - ctx->heap_start;

    /* Two regions, as on an AGA machine: 2 MB of Chip RAM above the
       library bases, Fast RAM above that. The Fast region leaves out the
       Kickstart ROM at $F80000, which holds just the ROM header: programs
       read the Kickstart version and the ROM size from it. */
    ctx->rgn[RGN_CHIP].start = ctx->heap_start;
    ctx->rgn[RGN_CHIP].end   = M68KEMU_CHIP_END;
    ctx->rgn[RGN_CHIP].free  = ctx->heap_start;
    ctx->rgn[RGN_CHIP].avail = M68KEMU_CHIP_END - ctx->heap_start;
    m68k_write32(ctx, ctx->heap_start + FB_NEXT, 0);
    m68k_write32(ctx, ctx->heap_start + FB_SIZE, M68KEMU_CHIP_END - ctx->heap_start);

    ctx->rgn[RGN_FAST].start = M68KEMU_CHIP_END;
    ctx->rgn[RGN_FAST].end   = ctx->heap_end;
    ctx->rgn[RGN_FAST].free  = M68KEMU_CHIP_END;
    ctx->rgn[RGN_FAST].avail = heap_size - (M68KEMU_CHIP_END - ctx->heap_start) - M68KEMU_ROM_SIZE;
    m68k_write32(ctx, M68KEMU_CHIP_END + FB_NEXT, M68KEMU_ROM_END);
    m68k_write32(ctx, M68KEMU_CHIP_END + FB_SIZE, M68KEMU_ROM_BASE - M68KEMU_CHIP_END);
    m68k_write32(ctx, M68KEMU_ROM_END + FB_NEXT, 0);
    m68k_write32(ctx, M68KEMU_ROM_END + FB_SIZE, ctx->heap_end - M68KEMU_ROM_END);

    m68k_write32(ctx, M68KEMU_ROM_BASE + 0, 0x11144ef9);
    m68k_write32(ctx, M68KEMU_ROM_BASE + 4, 0x00f80002);
    m68k_write32(ctx, M68KEMU_ROM_BASE + 8, 0x0000ffff);
    m68k_write16(ctx, M68KEMU_ROM_BASE + 12, SysBase->LibNode.lib_Version);
    m68k_write16(ctx, M68KEMU_ROM_BASE + 14, SysBase->LibNode.lib_Revision);
    m68k_write32(ctx, M68KEMU_ROM_END - 20, M68KEMU_ROM_SIZE);     /* ROM size, as at $FFFFEC */

    return ctx;
}

void M68KEmu_DestroyContext(struct M68KEmuContext *ctx)
{
    if (!ctx) return;
    m68k_vbl_close(ctx);
    M68KEmu_FreeNestCpus(ctx);
    /* notifications the program left running would signal a port that is gone */
    for (UWORD i = 0; i < ctx->num_notify; i++)
    {
        EndNotify(ctx->notify[i].nr);
        FreeVec(ctx->notify[i].nr->nr_Name);
        FreeVec(ctx->notify[i].nr);
    }
    ctx->num_notify = 0;
    /* pattern matches the program did not end hold directory locks */
    for (int i = 0; i < 16; i++)
        if (ctx->anchors[i].native)
        {
            MatchEnd(ctx->anchors[i].native);
            FreeVec(ctx->anchors[i].native);
            ctx->anchors[i].native = NULL;
        }
    if (ctx->handles) FreeVec(ctx->handles);
    m68k_boopsi_cleanup(ctx);
    for (int i = 0; i < 32; i++)
        FreeVec(ctx->gfx_owned[i]);
    for (int i = 0; i < 32; i++)
        if (ctx->own_rps[i])
            FreeVec(ctx->own_rps[i]);
    if (ctx->mem) FreeMem(ctx->mem, ctx->mem_size);
    FreeMem(ctx, sizeof(struct M68KEmuContext));
}

/* Each region's 68k MemHeader (see shadow_init_execbase) follows its free
   list: the first chunk and the free byte count */
static void heap_publish(struct M68KEmuContext *ctx, struct M68KHeapRegion *r)
{
    if (r->mh)
    {
        m68k_write32(ctx, r->mh + M68K_MH_FIRST, r->free);
        m68k_write32(ctx, r->mh + M68K_MH_FREE, r->avail);
    }
}

struct M68KHeapRegion *M68KEmu_HeapRegion(struct M68KEmuContext *ctx, ULONG addr)
{
    return &ctx->rgn[addr < M68KEMU_CHIP_END ? RGN_CHIP : RGN_FAST];
}

static ULONG heap_largest(struct M68KEmuContext *ctx, struct M68KHeapRegion *r)
{
    ULONG largest = 0, c, n = 0;

    for (c = r->free; c && n < 100000; c = m68k_read32(ctx, c + FB_NEXT), n++)
    {
        ULONG bs = m68k_read32(ctx, c + FB_SIZE);
        if (bs > largest) largest = bs;
    }
    return largest > M68KEMU_HEAP_HDR ? largest - M68KEMU_HEAP_HDR : 0;
}

/* AvailMem() for the regions requirements select */
ULONG M68KEmu_HeapAvail(struct M68KEmuContext *ctx, ULONG reqs)
{
    ULONG res = 0;
    int i;

    for (i = 0; i < 2; i++)
    {
        struct M68KHeapRegion *r = &ctx->rgn[i];
        ULONG v;

        if ((reqs & MEMF_CHIP) && i != RGN_CHIP) continue;
        if ((reqs & MEMF_FAST) && i != RGN_FAST) continue;
        if (reqs & MEMF_TOTAL)
            v = r->end - r->start - (i == RGN_FAST ? M68KEMU_ROM_SIZE : 0);
        else if (reqs & MEMF_LARGEST)
            v = heap_largest(ctx, r);
        else
            v = r->avail;
        if (reqs & MEMF_LARGEST)
            res = v > res ? v : res;
        else
            res += v;
    }
    return res;
}

/* AllocAbs(): the block at a fixed address, header included, if that range
   is free; 0 otherwise, as exec answers for memory that is taken or absent */
ULONG M68KEmu_HeapAllocAbs(struct M68KEmuContext *ctx, ULONG size, ULONG addr)
{
    struct M68KHeapRegion *r;
    ULONG blk, end, prev = 0, cur;

    if (!size || addr < ctx->heap_start + M68KEMU_HEAP_HDR || addr >= ctx->heap_end)
        return 0;
    r = M68KEmu_HeapRegion(ctx, addr);
    blk = (addr - M68KEMU_HEAP_HDR) & ~3u;
    end = (addr + size + 3) & ~3u;

    for (cur = r->free; cur; prev = cur, cur = m68k_read32(ctx, cur + FB_NEXT))
    {
        ULONG csize = m68k_read32(ctx, cur + FB_SIZE), cend = cur + csize;
        ULONG next = m68k_read32(ctx, cur + FB_NEXT);

        if (blk < cur || end > cend)
            continue;
        /* the part after the block stays free, if big enough */
        if (cend - end >= M68KEMU_HEAP_MIN_SPLIT)
        {
            m68k_write32(ctx, end + FB_NEXT, next);
            m68k_write32(ctx, end + FB_SIZE, cend - end);
            next = end;
        }
        else
            end = cend;
        /* and so does the part before it */
        if (blk - cur >= M68KEMU_HEAP_MIN_SPLIT)
        {
            m68k_write32(ctx, cur + FB_SIZE, blk - cur);
            m68k_write32(ctx, cur + FB_NEXT, next);
        }
        else
        {
            blk = cur;
            if (prev)
                m68k_write32(ctx, prev + FB_NEXT, next);
            else
                r->free = next;
        }
        m68k_write32(ctx, blk, end - blk);
        r->avail -= end - blk;
        heap_publish(ctx, r);
        return addr;
    }
    return 0;
}

/* Free blocks are Amiga MemChunks (next, size), so a region's free list is
   its MemHeader's chunk list; an allocated block starts with its size. */
static ULONG heap_alloc_in(struct M68KEmuContext *ctx, struct M68KHeapRegion *r,
                           ULONG size, ULONG needed, ULONG requirements)
{
    ULONG prev = 0, cur = r->free;

    while (cur)
    {
        ULONG blk_size = m68k_read32(ctx, cur + FB_SIZE);
        ULONG blk_next = m68k_read32(ctx, cur + FB_NEXT);

        if (blk_size >= needed)
        {
            ULONG remainder = blk_size - needed;

            if (remainder >= M68KEMU_HEAP_MIN_SPLIT)
            {
                /* Split: keep remainder as free, allocate from end */
                m68k_write32(ctx, cur + FB_SIZE, remainder);
                m68k_write32(ctx, cur + remainder, needed);
                if (requirements & MEMF_CLEAR)
                    memset(ctx->mem + cur + remainder + M68KEMU_HEAP_HDR, 0, size);
                r->avail -= needed;
                heap_publish(ctx, r);
                return cur + remainder + M68KEMU_HEAP_HDR;
            }
            /* Use whole block */
            needed = blk_size;
            if (prev)
                m68k_write32(ctx, prev + FB_NEXT, blk_next);
            else
                r->free = blk_next;
            m68k_write32(ctx, cur, needed);
            if (requirements & MEMF_CLEAR)
                memset(ctx->mem + cur + M68KEMU_HEAP_HDR, 0, needed - M68KEMU_HEAP_HDR);
            r->avail -= needed;
            heap_publish(ctx, r);
            return cur + M68KEMU_HEAP_HDR;
        }
        prev = cur;
        cur  = blk_next;
    }
    return 0;
}

/* MEMF_CHIP comes from Chip RAM only, MEMF_FAST from Fast RAM only; other
   requests prefer Fast RAM, as exec's MemList order does */
ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx, ULONG size, ULONG requirements)
{
    ULONG needed, a;

    size   = (size + 3) & ~3;
    needed = size + M68KEMU_HEAP_HDR;  /* header */

    if (!(requirements & MEMF_CHIP) &&
        (a = heap_alloc_in(ctx, &ctx->rgn[RGN_FAST], size, needed, requirements)))
        return a;
    if (!(requirements & MEMF_FAST) &&
        (a = heap_alloc_in(ctx, &ctx->rgn[RGN_CHIP], size, needed, requirements)))
        return a;
    bug("[m68kemu] HeapAlloc(%lu, %08lx) failed: chip %lu free (largest %lu), fast %lu free (largest %lu)\n",
        (unsigned long)size, (unsigned long)requirements,
        (unsigned long)ctx->rgn[RGN_CHIP].avail, (unsigned long)heap_largest(ctx, &ctx->rgn[RGN_CHIP]),
        (unsigned long)ctx->rgn[RGN_FAST].avail, (unsigned long)heap_largest(ctx, &ctx->rgn[RGN_FAST]));
    return 0;
}

void M68KEmu_HeapFree(struct M68KEmuContext *ctx, ULONG addr, ULONG size)
{
    struct M68KHeapRegion *r;
    ULONG blk, blk_size, prev, cur;

    if (!addr) return;

    blk      = addr - M68KEMU_HEAP_HDR;
    /* Only a block this heap handed out may come back: an address outside
       the heap, a size that runs past it, or a block already on the free
       list would wreck the list (the program's FreeMem() takes any pointer) */
    if (addr < ctx->heap_start + M68KEMU_HEAP_HDR || addr >= ctx->heap_end ||
        (addr >= M68KEMU_ROM_BASE && addr < M68KEMU_ROM_END + M68KEMU_HEAP_HDR))
    {
        if (ctx->trace_calls) bug("[m68kemu] FreeMem of %08lx (size %lu): not heap memory, ignored\n", (unsigned long)addr, (unsigned long)size);
        return;
    }
    r = M68KEmu_HeapRegion(ctx, blk);
    blk_size = m68k_read32(ctx, blk);
    if (blk_size < M68KEMU_HEAP_HDR || blk + blk_size > r->end)
    {
        if (ctx->trace_calls) bug("[m68kemu] FreeMem of %08lx: bad block header %08lx, ignored\n", (unsigned long)addr, (unsigned long)blk_size);
        return;
    }
    for (cur = r->free; cur; cur = m68k_read32(ctx, cur + FB_NEXT))
    {
        ULONG cs = m68k_read32(ctx, cur + FB_SIZE);
        if (blk >= cur && blk < cur + cs)
        {
            if (ctx->trace_calls) bug("[m68kemu] FreeMem of %08lx: already free, ignored\n", (unsigned long)addr);
            return;
        }
        if (cur > blk) break;
    }
    r->avail += blk_size;

    /* Insert in address order */
    prev = 0;
    cur  = r->free;
    while (cur && cur < blk)
    {
        prev = cur;
        cur  = m68k_read32(ctx, cur + FB_NEXT);
    }

    m68k_write32(ctx, blk + FB_SIZE, blk_size);
    m68k_write32(ctx, blk + FB_NEXT, cur);

    if (prev)
        m68k_write32(ctx, prev + FB_NEXT, blk);
    else
        r->free = blk;

    /* Coalesce forward */
    if (cur && (blk + blk_size) == cur)
    {
        ULONG cur_size = m68k_read32(ctx, cur + FB_SIZE);
        m68k_write32(ctx, blk + FB_SIZE, blk_size + cur_size);
        m68k_write32(ctx, blk + FB_NEXT, m68k_read32(ctx, cur + FB_NEXT));
        blk_size += cur_size;
    }

    /* Coalesce backward */
    if (prev)
    {
        ULONG prev_size = m68k_read32(ctx, prev + FB_SIZE);
        if ((prev + prev_size) == blk)
        {
            m68k_write32(ctx, prev + FB_SIZE, prev_size + blk_size);
            m68k_write32(ctx, prev + FB_NEXT, m68k_read32(ctx, blk + FB_NEXT));
        }
    }
    heap_publish(ctx, r);
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

        /* as LoadSeg() lays it out: ULONG size, BPTR next, data - programs
           walk their own seglist through the link before their code */
        m68k_addr = M68KEmu_HeapAlloc(ctx, data_size + 8, MEMF_CLEAR);
        if (!m68k_addr)
        {
            FreeVec(host);
            return 0;
        }
        m68k_write32(ctx, m68k_addr, data_size + 8);
        if (i > 0)
            m68k_write32(ctx, cont[i - 1] - 4, (m68k_addr + 4) >> 2);
        m68k_addr += 8;

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

    /* Programs read fields of the well-known bases (GfxBase->ActiView,
       IntuitionBase->FirstScreen, DosLibrary->dl_Root): give those bases
       their full m68k layout, synced from the native library. */
    static const struct { const char *lib; const char *layout; } base_layouts[] =
    {
        { "graphics.library",  "GfxBase"       },
        { "intuition.library", "IntuitionBase" },
        { "dos.library",       "DosLibrary"    },
        { NULL, NULL }
    };
    const struct M68KStructLayout *base_layout = NULL;
    ULONG base_size = (lib_id == 0) ? M68K_SIZEOF_EXECBASE : M68K_SIZEOF_LIBRARY;

    if (name && lib_id != 0)
    {
        for (i = 0; base_layouts[i].lib; i++)
            if (strcmp(name, base_layouts[i].lib) == 0)
            {
                base_layout = shadow_find_layout(base_layouts[i].layout);
                if (base_layout && base_layout->m68k_size > base_size)
                    base_size = base_layout->m68k_size;
                break;
            }
    }

    vectors_size = (ULONG)num_vectors * M68K_JT_SLOT_SIZE;
    total_size   = vectors_size + base_size;

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
        if (native && base_layout)
        {
            shadow_register(ctx, base_addr, native, base_layout->shadow_type);
            shadow_sync(ctx, base_layout, base_addr, native);
        }
        if (native)
            CloseLibrary(native);
        m68k_write16(ctx, base_addr + 8, (UWORD)NT_LIBRARY << 8);     /* ln_Type, ln_Pri */
        m68k_write16(ctx, base_addr + 16, (UWORD)vectors_size);       /* lib_NegSize */
        m68k_write16(ctx, base_addr + 18, (UWORD)base_size);          /* lib_PosSize */
        m68k_write16(ctx, base_addr + M68K_LIB_VERSION, ver);
        m68k_write16(ctx, base_addr + M68K_LIB_REVISION, rev);
    }

    ctx->libs[ctx->num_libs].m68k_addr   = base_addr;
    ctx->libs[ctx->num_libs].pos_size    = base_size;
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
    m68k_publish_base(ctx, base_addr, name);

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
    ULONG *hunk_reqs = NULL;
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
    hunk_reqs  = (ULONG *)AllocMem(num_hunks * sizeof(ULONG), MEMF_CLEAR);
    if (!hunk_sizes || !hunk_addrs || !hunk_reqs) goto fail;

    for (i = 0; i < num_hunks; i++)
    {
        ULONG v, reqs;

        if (p + 4 > end) goto fail;
        v = rd32(&p);
        /* bits 30/31: Chip or Fast RAM; both set, the requirements follow */
        reqs = (v >> 30) == 1 ? MEMF_CHIP : (v >> 30) == 2 ? MEMF_FAST : 0;
        if ((v >> 30) == 3)
        {
            if (p + 4 > end) goto fail;
            reqs = rd32(&p) & (MEMF_CHIP | MEMF_FAST);
        }
        hunk_sizes[i] = (v & HUNK_TYPE_MASK) * 4;
        hunk_reqs[i] = reqs;
    }

    /* Allocate space for each hunk in containment memory, cleared: BSS and
       the part of a hunk its block does not fill read as zero */
    for (i = 0; i < num_hunks; i++)
    {
        ULONG addr = M68KEmu_HeapAlloc(ctx, hunk_sizes[i], hunk_reqs[i]);
        if (!addr && hunk_reqs[i]) addr = M68KEmu_HeapAlloc(ctx, hunk_sizes[i], 0);
        if (!addr) goto fail;
        memset(ctx->mem + addr, 0, hunk_sizes[i]);
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
            ULONG longs = rd32(&p) & HUNK_TYPE_MASK;
            ULONG bytes = longs * 4;
            if (p + bytes > end || i >= num_hunks || bytes > hunk_sizes[i]) goto fail;

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
        else if (type == HUNK_RELOC32SHORT || type == HUNK_DREL32 || type == HUNK_RELRELOC32)
        {
            /* 16-bit counts, hunk numbers and offsets, padded to a long;
               RELRELOC32 adds the distance to the target hunk */
            const UBYTE *start = p;
            for (;;)
            {
                ULONG count, ref_hunk, j;

                if (p + 2 > end) goto fail;
                count = (p[0] << 8) | p[1]; p += 2;
                if (count == 0) break;
                if (p + 2 > end) goto fail;
                ref_hunk = (p[0] << 8) | p[1]; p += 2;
                if (ref_hunk >= num_hunks || i >= num_hunks) goto fail;
                for (j = 0; j < count; j++)
                {
                    ULONG offset;

                    if (p + 2 > end) goto fail;
                    offset = (p[0] << 8) | p[1]; p += 2;
                    if (offset + 3 < hunk_sizes[i])
                    {
                        ULONG addr = hunk_addrs[i] + offset;
                        ULONG add = type == HUNK_RELRELOC32 ? hunk_addrs[ref_hunk] - hunk_addrs[i] : hunk_addrs[ref_hunk];
                        m68k_write32(ctx, addr, m68k_read32(ctx, addr) + add);
                    }
                }
            }
            if ((p - start) & 2)
                p += 2;
        }
        else if (type == HUNK_SYMBOL)
        {
            for (;;)
            {
                ULONG n;

                if (p + 4 > end) goto fail;
                n = rd32(&p) & 0x00FFFFFF;
                if (n == 0) break;
                if (p + 4 * (n + 1) > end) goto fail;
                p += 4 * (n + 1);
            }
        }
        else if (type == HUNK_DEBUG || type == HUNK_NAME)
        {
            ULONG n;

            if (p + 4 > end) goto fail;
            n = rd32(&p);
            if (p + 4 * n > end) goto fail;
            p += 4 * n;
        }
        else if (type == HUNK_END)
        {
            i++;
        }
        else
        {
            m68k_log(ctx, "hunk file: block type %lx is not supported", (unsigned long)type);
            goto fail;
        }
    }

    ctx->entry_point = entry_point;
    ctx->seg_start   = entry_point;

    /* The span this file occupies: the heap allocates top down, so a
       later hunk can sit below the first one */
    ctx->load_lo = 0xFFFFFFFF;
    ctx->load_hi = 0;
    for (i = 0; i < num_hunks; i++)
    {
        if (hunk_addrs[i] < ctx->load_lo) ctx->load_lo = hunk_addrs[i];
        if (hunk_addrs[i] + hunk_sizes[i] > ctx->load_hi) ctx->load_hi = hunk_addrs[i] + hunk_sizes[i];
    }

    FreeMem(hunk_sizes, num_hunks * sizeof(ULONG));
    FreeMem(hunk_addrs, num_hunks * sizeof(ULONG));
    FreeMem(hunk_reqs, num_hunks * sizeof(ULONG));
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
    if (hunk_reqs) FreeMem(hunk_reqs, num_hunks * sizeof(ULONG));
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

void m68k_low_write_trap(struct M68KEmuContext *ctx, ULONG addr, const char *who)
{
    if (ctx->low_trap_count++ < 8 && ctx->trace_calls)
        bug("[m68kemu] %s wrote into the vector table at %08x (dropped)\n", who, (unsigned)addr);
}

/* ── Vertical blank clock ────────────────────────────────────────────
   A 50 Hz tick from timer.device's E clock; the CPU loop asks every few
   thousand instructions. */

#include <devices/timer.h>
#include <proto/timer.h>

BOOL m68k_vbl_due(struct M68KEmuContext *ctx)
{
    struct EClockVal ev;
    struct Device *TimerBase;
    ULONG elapsed;

    if (!ctx->vbl_timerio)
    {
        struct MsgPort *port = CreateMsgPort();
        struct timerequest *req = port ? (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest)) : NULL;
        if (!req || OpenDevice((CONST_STRPTR)"timer.device", UNIT_ECLOCK, (struct IORequest *)req, 0) != 0)
        {
            bug("[m68kemu] no timer.device for the vertical blank clock\n");
            if (req) DeleteIORequest((struct IORequest *)req);
            if (port) DeleteMsgPort(port);
            return FALSE;
        }
        ctx->vbl_timerport = port;
        ctx->vbl_timerio = req;
        TimerBase = req->tr_node.io_Device;
        ctx->vbl_efreq = ReadEClock(&ev);
        ctx->vbl_last_lo = ev.ev_lo;
        ctx->vbl_last_hi = ev.ev_hi;
        return FALSE;
    }
    TimerBase = ((struct timerequest *)ctx->vbl_timerio)->tr_node.io_Device;
    ReadEClock(&ev);
    elapsed = ev.ev_lo - ctx->vbl_last_lo;          /* wraps correctly for short intervals */
    if (ctx->vbl_efreq == 0)
    {
        bug("[m68kemu] VBL clock: ReadEClock reports frequency 0\n");
        ctx->vbl_efreq = 709379;
    }
    {
        static ULONG calls = 0;
        if ((++calls & 0x3FFF) == 0 && ctx->trace_calls)
            bug("[m68kemu] VBL clock: call %lu elapsed %lu ev_lo %lu last %lu\n", (unsigned long)calls,
                (unsigned long)elapsed, (unsigned long)ev.ev_lo, (unsigned long)ctx->vbl_last_lo);
    }
    if (elapsed < ctx->vbl_efreq / 50)
        return FALSE;
    if (ctx->vbl_count < 8 && ctx->trace_calls)
        bug("[m68kemu] VBL clock: due, elapsed %lu E ticks (freq %lu)\n", (unsigned long)elapsed, (unsigned long)ctx->vbl_efreq);
    ctx->vbl_last_lo = ev.ev_lo;
    ctx->vbl_last_hi = ev.ev_hi;
    return TRUE;
}

void m68k_vbl_close(struct M68KEmuContext *ctx)
{
    if (ctx->vbl_timerio)
    {
        CloseDevice((struct IORequest *)ctx->vbl_timerio);
        DeleteIORequest((struct IORequest *)ctx->vbl_timerio);
        ctx->vbl_timerio = NULL;
    }
    if (ctx->vbl_timerport)
    {
        DeleteMsgPort((struct MsgPort *)ctx->vbl_timerport);
        ctx->vbl_timerport = NULL;
    }
}

/* Current E clock (ev_lo) for the CIA model; opens the timer like m68k_vbl_due */
ULONG m68k_eclock_now(struct M68KEmuContext *ctx, ULONG *freq)
{
    struct EClockVal ev;
    struct Device *TimerBase;

    if (!ctx->vbl_timerio)
    {
        (void)m68k_vbl_due(ctx);
        if (!ctx->vbl_timerio)
        {
            if (freq) *freq = 709379;
            return 0;
        }
    }
    TimerBase = ((struct timerequest *)ctx->vbl_timerio)->tr_node.io_Device;
    ReadEClock(&ev);
    if (freq) *freq = ctx->vbl_efreq ? ctx->vbl_efreq : 709379;
    return ev.ev_lo;
}

/* the last library calls (M68KEMU_TRACE=end), string compares only counted:
   a loop that walks a list hides the calls that drive it */
void m68k_dump_callring(struct M68KEmuContext *ctx)
{
    ULONG n = ctx->callring_pos < 2048 ? ctx->callring_pos : 2048, cmp = 0, shown = 0;

    for (ULONG i = ctx->callring_pos - n; i < ctx->callring_pos && shown < 200; i++)
    {
        ULONG k = i & 2047;
        if (!strcmp(ctx->callring[k].lib, "utility.library") &&
            (ctx->callring[k].lvo == 162 || ctx->callring[k].lvo == 168))
        {
            cmp++;
            continue;
        }
        if (cmp)
            bug("[m68kemu]   (%lu string compares)\n", (unsigned long)cmp);
        cmp = 0;
        shown++;
        bug("[m68kemu]   %s -%lu d0 %08lx d1 %08lx a0 %08lx a1 %08lx -> %08lx (from %08lx)\n",
            ctx->callring[k].lib, (unsigned long)ctx->callring[k].lvo,
            (unsigned long)ctx->callring[k].d0, (unsigned long)ctx->callring[k].d1,
            (unsigned long)ctx->callring[k].a0, (unsigned long)ctx->callring[k].a1,
            (unsigned long)ctx->callring[k].ret, (unsigned long)ctx->callring[k].from);
    }
    if (cmp)
        bug("[m68kemu]   (%lu string compares)\n", (unsigned long)cmp);
}

/* Diagnostic watchdog (M68KEMU_TRACE set): a native process that prints
   where the emulated CPU is every two seconds, so a stall inside one
   instruction shows up as a frozen count and PC */
#include <dos/dostags.h>
static struct M68KEmuContext *wd_ctx;
static void wd_entry(void)
{
    struct M68KEmuContext *ctx = wd_ctx;
    ULONG last = 0xFFFFFFFF, stuck = 0, ticks = 0;
    while (!ctx->watchdog_quit)
    {
        ULONG ic, pc;
        Delay(100);
        if (ctx->watchdog_quit) break;
        /* M68KEMU_TRACE=end: once the program has sat in one call for ten
           seconds, the calls that led there */
        if (ctx->trace_end && !ctx->trace_calls)
        {
            ic = ctx->icount_lo;
            stuck = ic == last ? stuck + 1 : 0;
            last = ic;
            if (stuck == 5 || ++ticks == 30)
            {
                bug("[m68kemu] %s %s, after %lu calls:\n",
                    ctx->program_name[0] ? ctx->program_name : "68k program",
                    stuck == 5 ? "has been in one library call for 10 s" : "is still running at 60 s",
                    (unsigned long)ctx->callring_pos);
                m68k_dump_callring(ctx);
                pc = ctx->pc_ring[(ctx->pc_ring_pos - 1) & 511];
                bug("[m68kemu]   now at pc %08lx (%04x %04x)\n", (unsigned long)pc,
                    (unsigned)m68k_read16(ctx, pc), (unsigned)m68k_read16(ctx, pc + 2));
                for (UWORD l = 0; l < ctx->num_libs; l++)
                    if (pc >= ctx->libs[l].jt_start && pc < ctx->libs[l].m68k_addr)
                        bug("[m68kemu]   in %s -%lu, called from %08lx\n", ctx->libs[l].name[0] ? ctx->libs[l].name : "(library)",
                            (unsigned long)(ctx->libs[l].m68k_addr - pc), (unsigned long)ctx->pc_ring[(ctx->pc_ring_pos - 2) & 511]);
            }
            continue;
        }
        if (!ctx->trace_calls) continue;           /* quiet until the trace starts */
        ic = ctx->icount_lo;
        pc = ctx->pc_ring[(ctx->pc_ring_pos - 1) & 511];
        bug("[m68kemu] WATCHDOG icount %lu%s pc %08lx op %04x %04x vbl %lu irq %d code %08lx\n",
            (unsigned long)ic, ic == last ? " (STUCK)" : "", (unsigned long)pc,
            (unsigned)m68k_read16(ctx, pc), (unsigned)m68k_read16(ctx, pc + 2),
            (unsigned long)ctx->vbl_count, (int)ctx->vbl_irq, (unsigned long)ctx->vbl_code);
        last = ic;
    }
    ctx->watchdog_quit = FALSE;
}

void m68k_watchdog_start(struct M68KEmuContext *ctx)
{
    wd_ctx = ctx;
    ctx->watchdog_quit = FALSE;
    ctx->watchdog_on = CreateNewProcTags(NP_Entry, (IPTR)wd_entry, NP_Name, (IPTR)"m68kemu watchdog",
                                         NP_Priority, 5, TAG_DONE) != NULL;
}

void m68k_watchdog_stop(struct M68KEmuContext *ctx)
{
    if (!ctx->watchdog_on) return;
    ctx->watchdog_quit = TRUE;
    while (ctx->watchdog_quit) Delay(10);
    ctx->watchdog_on = FALSE;
}

/* Messages about a program (it failed, a library is missing): to the
   system log, which LogDump and the log viewers read, so nothing paints
   over the screen; to the debug output as well while M68KEMU_TRACE is set. */
#include <resources/log.h>
#include <proto/log.h>
#include <stdarg.h>
#include <stdio.h>
static struct Library *m68k_LogResBase;
static APTR m68k_loghandle;
static struct Node m68k_lognode = { .ln_Name = (char *)"m68kemu" };
#define __LOG_LIBBASE m68k_LogResBase

void m68k_log(struct M68KEmuContext *ctx, const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (ctx && ctx->trace_calls)
        bug("[m68kemu] %s\n", buf);
    Forbid();
    if (!m68k_LogResBase)
    {
        m68k_LogResBase = OpenResource("log.resource");
        if (m68k_LogResBase)
            m68k_loghandle = logInitialise(&m68k_lognode);
    }
    Permit();
    if (m68k_LogResBase && m68k_loghandle)
    {
        /* the message is already formatted: plain text, no format arguments */
        char *c;
        for (c = buf; *c; c++)
            if (*c == '%') *c = '?';
        logAddEntryA(LOGF_Flag_Type_Error | 20, m68k_loghandle, (CONST_STRPTR)"", (CONST_STRPTR)"m68kemu", 0,
                     (CONST_STRPTR)buf, (RAWARG)NULL);
    }
    else if (!ctx || !ctx->trace_calls)
        bug("[m68kemu] %s\n", buf);
}
