/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    GPU memory for the V3D: system RAM, not the VideoCore partition.

    Buffer objects used to be firmware allocations (ALLOCMEM/LOCKMEM) out of
    the gpu_mem= partition, inherited from vc4gallium's VideoCore IV model.
    That caps the driver at the firmware's 512MB maximum, and Doom 3 wants
    more than that in textures alone (measured: 5395 BOs holding 454MB, with
    every texture already downsized to 256px).

    A V3D 4.2 has its own MMU, so it needs no firmware memory at all - any
    physical page will do, which is how Linux's v3d driver works too. Arenas
    come from AllocMem, are remapped Normal Non-Cacheable so that the CPU
    and the GPU agree on memory contents without a single cache operation
    (exactly the property the uncached VideoCore alias used to give us), and
    are suballocated with exec's own Allocate/Deallocate over a private
    MemHeader.

    Two invariants keep that suballocation page-granular with no aligning
    wrapper: the arena base is 4K-aligned, and every request is a whole
    number of 4K pages. stdAlloc() hands out the FRONT of a free chunk and
    splits the remainder at base+size (rom/exec/memory.c), so every chunk
    boundary stays 4K-aligned for the arena's whole life.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <aros/kernel.h>
#include <proto/exec.h>

extern APTR KernelBase;
#include <proto/kernel.h>

#include "v3d_intern.h"

/* Big enough for tens rather than hundreds of arenas, small enough that
 * AllocMem still finds it contiguous. */
#define V3D_ARENA_SIZE  (32 << 20)

/*
 * 2MB-aligned and -sized so KrnMapGlobal covers an arena with pure 2MB
 * blocks. A 4K-aligned one forces an L3 table at either end, and the
 * kernel never frees an emptied table page (mmu_unmap_range), so those
 * would leak per GL session.
 */
#define V3D_BLOCK       (2 << 20)
#define V3D_BLOCK_MASK  (V3D_BLOCK - 1)

#define V3D_PAGE        4096
#define V3D_PAGE_MASK   (V3D_PAGE - 1)

/* GPU VAs are the physical addresses themselves and the V3D MMU's VA
 * space is 32-bit, so a page above 4GB cannot be mapped. */
#define V3D_PA_LIMIT    0x100000000ULL

static void arena_free(struct V3DData *sd, struct V3DArena *a)
{
    /* Back to cacheable: the next owner expects ordinary RAM. Nothing to
     * invalidate - non-cacheable accesses never allocated a line. */
    KrnMapGlobal(a->base, a->base, a->size, MAP_Readable | MAP_Writable);

    sd->arena_bytes -= a->size;
    FreeMem(a->raw, a->raw_size);
    FreeMem(a, sizeof(struct V3DArena));
}

static struct V3DArena *arena_new(struct V3DData *sd, ULONG want)
{
    struct V3DArena *a;
    APTR raw;
    IPTR base;
    ULONG raw_size;

    want = (want + V3D_BLOCK_MASK) & ~(ULONG)V3D_BLOCK_MASK;
    raw_size = want + V3D_BLOCK;

    a = AllocMem(sizeof(struct V3DArena), MEMF_ANY | MEMF_CLEAR);
    if (!a)
        return NULL;

    raw = AllocMem(raw_size, MEMF_ANY);
    if (!raw)
    {
        FreeMem(a, sizeof(struct V3DArena));
        return NULL;
    }

    base = ((IPTR)raw + V3D_BLOCK_MASK) & ~(IPTR)V3D_BLOCK_MASK;

    if ((UQUAD)base + want > V3D_PA_LIMIT)
    {
        bug("[V3D] arena at 0x%p is above the 4GB the GPU can map\n",
            (void *)base);
        FreeMem(raw, raw_size);
        FreeMem(a, sizeof(struct V3DArena));
        return NULL;
    }

    /* Cacheable -> Normal-NC. Clean and invalidate first, or a dirty
     * line from the previous owner lands on top of GPU data later. */
    CacheClearE((APTR)base, want, CACRF_ClearD);

    if (!KrnMapGlobal((APTR)base, (APTR)base, want,
                      MAP_Readable | MAP_Writable | MAP_WriteThrough))
    {
        bug("[V3D] arena at 0x%p could not be mapped Normal-NC\n",
            (void *)base);
        FreeMem(raw, raw_size);
        FreeMem(a, sizeof(struct V3DArena));
        return NULL;
    }

    a->raw      = raw;
    a->raw_size = raw_size;
    a->base     = (APTR)base;
    a->size     = want;

    /* A private heap over the arena. The header stays in cached memory;
     * only the free-chunk list lives in the uncached arena. */
    a->mh.mh_Node.ln_Type   = NT_MEMORY;
    a->mh.mh_Node.ln_Name   = "v3d gpu";
    a->mh.mh_Attributes     = MEMF_ANY;
    a->mh.mh_Lower          = (APTR)base;
    a->mh.mh_Upper          = (APTR)(base + want);
    a->mh.mh_First          = (struct MemChunk *)base;
    a->mh.mh_First->mc_Next  = NULL;
    a->mh.mh_First->mc_Bytes = want;
    a->mh.mh_Free           = want;

    AddTail((struct List *)&sd->arenas, (struct Node *)&a->node);
    sd->arena_bytes += want;

    D(bug("[V3D] arena %u KB at 0x%p (%u KB total)\n",
          (unsigned)(want >> 10), (void *)base,
          (unsigned)(sd->arena_bytes >> 10)));

    return a;
}

/* Taken from the system, still free (fragmentation), and held by live
 * BOs - a refusal means nothing without those three. */
static void gpu_mem_report(struct V3DData *sd, ULONG failed_size)
{
    struct V3DArena *a;
    ULONG h, arenas = 0, free_bytes = 0, bo_live = 0, bo_bytes = 0;

    /* The first few carry the information; Mesa retries every draw. */
    if (++sd->oom_reports > 3 && (sd->oom_reports & 63) != 0)
        return;

    ForeachNode(&sd->arenas, a)
    {
        arenas++;
        free_bytes += a->mh.mh_Free;
    }

    for (h = 1; h < V3D_MAX_BOS; h++)
        if (sd->bo_table[h].refcount && sd->bo_table[h].gpu_handle)
        {
            bo_live++;
            bo_bytes += sd->bo_table[h].size;
        }

    bug("[V3D] OOM: %u KB refused. %u arenas holding %u KB, %u KB of that "
        "free; %u BOs, %u KB\n",
        (unsigned)(failed_size >> 10), (unsigned)arenas,
        (unsigned)(sd->arena_bytes >> 10), (unsigned)(free_bytes >> 10),
        (unsigned)bo_live, (unsigned)(bo_bytes >> 10));
}

/* The returned handle IS the address (identity-mapped arena). Only 4K
 * alignment is honoured, which is all any caller asks for. */
ULONG v3d_gpu_mem_alloc(struct V3DData *sd, ULONG size, ULONG align,
                        ULONG *out_paddr)
{
    struct V3DArena *a;
    APTR mem = NULL;

    (void)align;
    size = (size + V3D_PAGE_MASK) & ~(ULONG)V3D_PAGE_MASK;

    ObtainSemaphore(&sd->bo_lock);

    ForeachNode(&sd->arenas, a)
    {
        if (a->mh.mh_Free < size)
            continue;
        if ((mem = Allocate(&a->mh, size)) != NULL)
            break;
    }

    if (!mem)
    {
        a = arena_new(sd, size > V3D_ARENA_SIZE ? size : V3D_ARENA_SIZE);
        if (a)
            mem = Allocate(&a->mh, size);
    }

    if (!mem)
    {
        gpu_mem_report(sd, size);
        ReleaseSemaphore(&sd->bo_lock);
        return 0;
    }

    sd->gpu_mem_bytes += size;
    sd->gpu_mem_allocs++;
    ReleaseSemaphore(&sd->bo_lock);

    /* The firmware allocator zeroed on request and callers rely on it -
     * an all-zero page table is an all-invalid one. */
    memset(mem, 0, size);

    *out_paddr = (ULONG)(IPTR)mem;
    return (ULONG)(IPTR)mem;
}

void v3d_gpu_mem_free(struct V3DData *sd, ULONG gpu_handle, ULONG size)
{
    struct V3DArena *a;
    IPTR addr = (IPTR)gpu_handle;

    if (!gpu_handle)
        return;

    size = (size + V3D_PAGE_MASK) & ~(ULONG)V3D_PAGE_MASK;

    ObtainSemaphore(&sd->bo_lock);

    ForeachNode(&sd->arenas, a)
    {
        if (addr < (IPTR)a->base || addr >= (IPTR)a->base + a->size)
            continue;

        Deallocate(&a->mh, (APTR)addr, size);
        sd->gpu_mem_bytes -= size;
        sd->gpu_mem_allocs--;
        break;
    }

    ReleaseSemaphore(&sd->bo_lock);
}

/*
 * Session sweep, after the BO table is emptied: hand back every arena
 * nothing is using. Tests for empty rather than freeing the lot because
 * the page table, scratch page, overflow buffers and landing zone are
 * kept across sessions.
 */
void v3d_mem_release(struct V3DData *sd)
{
    struct V3DArena *a, *next;
    ULONG released = 0;

    ObtainSemaphore(&sd->bo_lock);

    ForeachNodeSafe(&sd->arenas, a, next)
    {
        if (a->mh.mh_Free != a->size)
            continue;
        Remove((struct Node *)&a->node);
        arena_free(sd, a);
        released++;
    }

    ReleaseSemaphore(&sd->bo_lock);

    if (released)
        D(bug("[V3D] released %u arenas, %u KB still held\n",
              (unsigned)released, (unsigned)(sd->arena_bytes >> 10)));
}
