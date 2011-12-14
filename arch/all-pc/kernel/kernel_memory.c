/*
 * Functions for dealing with Multiboot memory map.
 * This file override basic MemHeader creation functions in rom/kernel,
 * because if you have memory map you don't need them.
 * This code builds a fully-functional set of MemHeaders and MemChunks
 * based on memory map contents and physical breakout described in the array
 * of MemRegion structures.
 */

#include <aros/macros.h>
#include <aros/multiboot.h>
#include <exec/lists.h>
#include <exec/memory.h>

#include "kernel_mmap.h"

/*
 * Append a single chunk to a MemHeader.
 * If MemHeader address is not set, a MemHeader will be created in this chunk
 * with the parameters specified in MemRegion structure.
 * Returns the last MemChunk in the chain, for linking.
 */
static struct MemChunk *krnAddMemChunk(struct MemHeader **mhPtr, struct MemChunk *prev, IPTR start, IPTR end,
                                       IPTR mh_Start, const struct MemRegion *reg)
{
    struct MemChunk *mc;

    if (*mhPtr == NULL)
    {
        /* Align start address - who knows... */
        start = AROS_ROUNDUP2(start, sizeof(IPTR));

        /* Ignore the chunk if it's too small to place the MemHeader there */
        if (start > end)
            return NULL;
        if (end - start < sizeof(struct MemHeader))
            return NULL;

        /* Create MemHeader if it is not there yet */
        *mhPtr = (struct MemHeader *)start;
        start += sizeof(struct MemHeader);

        (*mhPtr)->mh_Node.ln_Name = reg->name;
        (*mhPtr)->mh_Node.ln_Type = NT_MEMORY;
        (*mhPtr)->mh_Node.ln_Pri  = reg->pri;
        (*mhPtr)->mh_Attributes   = reg->flags;
        (*mhPtr)->mh_Lower        = (APTR)mh_Start;
        (*mhPtr)->mh_First        = NULL;       /* We don't actually have any single MemChunk yet */
        (*mhPtr)->mh_Free         = 0;

        /* The next MemChunk will be linked to our MemHeader */
        prev = (struct MemChunk *)&(*mhPtr)->mh_First;
    }

    (*mhPtr)->mh_Upper = (APTR)end;

    /* MemChunk must start and end on aligned addresses */
    start = AROS_ROUNDUP2(start, MEMCHUNK_TOTAL);
    end   = AROS_ROUNDDOWN2(end, MEMCHUNK_TOTAL);

    /* If there is not enough space, skip this chunk */
    if (start > end)
        return prev;
    if (end - start < MEMCHUNK_TOTAL)
        return prev;

    mc = (struct MemChunk *)start;
    mc->mc_Next  = NULL;
    mc->mc_Bytes = end - start;

    /* Append this chunk to a MemHeader */
    prev->mc_Next = mc;
    (*mhPtr)->mh_Free += mc->mc_Bytes;

    return mc;
}

/*
 * The same as above, but aware of kickstart placement
 * klo - Lowest address of the kickstart region
 * khi - Next free address beyond the kickstart (kickstart highest address + 1)
 */
static struct MemChunk *krnAddKickChunk(struct MemHeader **mhPtr, struct MemChunk *prev, IPTR start, IPTR end,
                                        IPTR klo, IPTR khi, IPTR mh_Start, const struct MemRegion *reg)
{
    /* If the kickstart is placed outside of this region, just add it as it is */
    if ((klo >= end) || (khi <= start))
        return krnAddMemChunk(mhPtr, prev, start, end, mh_Start, reg);

    /* Have some usable space above the kickstart ? */
    if (klo > start)
        prev = krnAddMemChunk(mhPtr, prev, start, klo, mh_Start, reg);

    /* Have some usable space below the kickstart ? */
    if (khi < end)
        prev = krnAddMemChunk(mhPtr, prev, khi, end, mh_Start, reg);

    return prev;
}

/*
 * Build conventional memory lists out of multiboot memory map structure.
 * Will add all MemHeaders to the specified list in the same order they
 * were created, not in the priority one.
 * Memory breakup is specified by an array of MemRegion structures.
 *
 * It is suggested that:
 * 1. Addresses in both memory map and MemRegion structures
 *    are sorted in ascending order.
 * 2. MemRegion structures must specify continuous addresses range (i. e. it is guaranteed
 *    that none of memory map entries will happen to be between two MemRegions).
 */
void mmap_InitMemory(struct mb_mmap *mmap, unsigned long len, struct MinList *memList,
                     IPTR klo, IPTR khi, IPTR reserve, const struct MemRegion *reg)
{
    while (len >= sizeof(struct mb_mmap))
    {
        /* We don't have a header yet */
        struct MemHeader *mh = NULL;
        struct MemChunk *mc = NULL;
        IPTR phys_start = mmap->addr;
        IPTR end = 0;
        IPTR addr;

        /* Go to the first matching region */
        while (reg->end <= phys_start)
        {
            reg++;
            /* NULL name is a terminator */
            if (reg->name == NULL)
                return;
        }

        /* Adjust start address if needed */
        if (phys_start < reg->start)
            phys_start = reg->start;

        for (;;)
        {
#ifdef __i386__
            /* We are on i386, ignore high memory */
            if (mmap->addr_high)
                break;

            if (mmap->len_high)
                end = 0x80000000;
            else
#endif
                end  = mmap->addr + mmap->len;
            addr = mmap->addr;

            if (addr < reg->start)
            {
                /* 
                 * This region includes space from the previous MemHeader.
                 * Trim it.
                 */
                addr = reg->start;
            }
            else if (addr == 0)
            {
                /* Reserve requested space in zero page */
                addr = reserve;
            }

            /* Is the limit in the middle of current chunk ? */
            if (end > reg->end)
            {
                /* Add a partial chunk, up to limit */
                end = reg->end;

                if (mmap->type == MMAP_TYPE_RAM)
                    krnAddKickChunk(&mh, mc, addr, end, klo, khi, phys_start, reg);

                /* We will continue with the next region within this chunk */
                reg++;
                break;
            }

            /* Add the entire chunk */
            if (mmap->type == MMAP_TYPE_RAM)
                mc = krnAddKickChunk(&mh, mc, addr, end, klo, khi, phys_start, reg);

            /* Go to the next chunk */
            len -= mmap->size + 4;
            mmap = (struct mb_mmap *)(mmap->size + (IPTR)mmap + 4);

            /* If this was the last chunk, we are done */
            if (len < sizeof(struct mb_mmap))
                break;

            /*
             * If the next chunk is not a physical continuation of the previous one,
             * we break inner loop and start over again with a new MemHeader
             */
            if (mmap->addr != end)
                break;
        }

        /* Complete the MemHeader and add it to the list */
        if (mh)
        {
            mh->mh_Upper = (APTR)end;
            ADDTAIL(memList, mh);
        }

        /* If we reached the end of layout table, exit */
        if (reg->name == NULL)
            return;
    }
}

struct mb_mmap *mmap_FindRegion(IPTR addr, struct mb_mmap *mmap, unsigned long len)
{
    while (len >= sizeof(struct mb_mmap))
    {
        IPTR end;

#ifdef __i386__
        /* We are on i386, ignore high memory */
        if (mmap->addr_high)
            return NULL;

        if (mmap->len_high)
            end = 0x80000000;
        else
#endif
        end = mmap->addr + mmap->len;

        /* Returh chunk pointer if matches */
        if ((addr >= mmap->addr) && (addr < end))
            return mmap;

        /* Go to the next chunk */
        len -= mmap->size + 4;
        mmap = (struct mb_mmap *)(mmap->size + (IPTR)mmap + 4);
    }
    return NULL;
}

/* Validate the specified region via memory map */
BOOL mmap_ValidateRegion(unsigned long addr, unsigned long len, struct mb_mmap *mmap, unsigned long mmap_len)
{
    /* Locate a memory region */
    struct mb_mmap *region = mmap_FindRegion(addr, mmap, mmap_len);

    /* If it exists, and free for usage... */
    if (region && region->type == MMAP_TYPE_RAM)
    {
        IPTR end = region->addr + region->len;

        /* Make sure it covers the whole our specified area */
        if (addr + len < end)
            return TRUE;

    }
    return FALSE;
}
