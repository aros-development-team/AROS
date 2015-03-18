/*
    Copyright © 1995-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/memory.h>
#include <proto/exec.h>
#include <inttypes.h>
#include <aros/debug.h>

#include "intelG45_intern.h"
#include "intelG45_regs.h"

static void G45_AttachMemory(struct g45staticdata *sd, intptr_t physical,
    intptr_t virtual, intptr_t length);
static void G45_DetachMemory(struct g45staticdata *sd, intptr_t virtual,
    intptr_t length);
static APTR AllocAlignedMem(IPTR size, UWORD alignment);
static VOID FreeAlignedMem(APTR mem);

APTR AllocGfxMem(struct g45staticdata *sd, ULONG size)
{
    APTR result = NULL;
    struct MemHeader *header, *next_header, *gap_header = NULL, *tail;

    header = (APTR)sd->CardMem.mlh_Head;
    tail = (APTR)&sd->CardMem.mlh_Tail;

    while (header != tail && result == NULL)
    {
        Forbid();
        result = Allocate(header, size);
        Permit();

        next_header = (APTR)header->mh_Node.ln_Succ;
        if (next_header != tail)
        {
            if (gap_header == NULL
                && header->mh_Upper != next_header->mh_Lower)
                gap_header = header;
        }
        header = next_header;
    }

    if (result == NULL)
    {
        /* If a header with a gap after it wasn't found, we use the last one */
        if (gap_header == NULL)
            gap_header = (APTR)sd->CardMem.mlh_TailPred;

        Forbid();
        /* Choose first 16MB block that isn't represented by an existing
         * memory header */
        header = ObtainGfxMemory(sd,
            gap_header->mh_Upper - (APTR)sd->Card.Framebuffer, 16 * MBYTES,
            FALSE);
        if (header != NULL)
        {
            Insert((APTR)&sd->CardMem, &header->mh_Node,
                &gap_header->mh_Node);
            result = Allocate(header, size);
        }
        Permit();
    }

    return result;
}

VOID FreeGfxMem(struct g45staticdata *sd, APTR ptr, ULONG size)
{
    struct MemHeader *header, *tail;
    BOOL freed = FALSE;

    header = (APTR)sd->CardMem.mlh_Head;
    tail = (APTR)&sd->CardMem.mlh_Tail;

    while (header != tail && !freed)
    {
        /* Check if this is the header containing the memory to be freed */
        if (ptr >= header->mh_Lower && ptr < header->mh_Upper)
        {
            Forbid();
            Deallocate(header, ptr, size);
            Permit();

            freed = TRUE;

            /* Unmap memory from graphics chip if it's unused. We don't unmap
             * the first header because it's the initial BIOS-stolen memory */
            if (header->mh_Free == header->mh_Upper - header->mh_Lower
                && (struct MinNode *)header != sd->CardMem.mlh_Head)
            {
                Remove(&header->mh_Node);
                ReleaseGfxMemory(sd, header);
            }
        }

        if(!freed)
            header = (APTR)header->mh_Node.ln_Succ;
    }
}

intptr_t G45_VirtualToPhysical(struct g45staticdata *sd, intptr_t virtual)
{
    intptr_t page = virtual >> 12;      /* get page number */
    intptr_t result = -1;

    if (page >= 0 && page < sd->Card.GATT_size / 4)
    {
        uint32_t pte = readl(&sd->Card.GATT[page]);
        if (pte & 1)
        {
            result = pte & 0xfffff000;
            result |= virtual & 0xfff;
        }
    }

    return result;
}

struct MemHeader *ObtainGfxMemory(struct g45staticdata *sd, intptr_t virtual,
    intptr_t length, BOOL stolen)
{
    BOOL success = TRUE;
    APTR sys_mem = NULL;
    struct MemHeader *header;
    struct MemChunk *chunk;

    /* Ensure we don't over-extend graphics memory into scratch area at end
     * of aperture */
    if (virtual + length >= sd->ScratchArea)
    {
        length = sd->ScratchArea - virtual;
        if (length == 0)
            success = FALSE;
    }

    if (success)
    {
        /* Get memory (aligned to page size) */
        if (!stolen)
        {
            sys_mem = AllocAlignedMem(length, 4096);
            D(bug("[GMA] Got %08x\n", sys_mem));
        }

        header = AllocVec(sizeof(struct MemHeader), MEMF_CLEAR);
        if (sys_mem == NULL && !stolen || header == NULL)
            success = FALSE;
    }

    if (success)
    {
        D(bug("[GMA] Mapping system memory %08x to graphics virtual %08x "
            "with size %08x\n", sys_mem, virtual, length));
        if (!stolen)
        {
            /* Mirror allocated system memory in graphics memory window */
            G45_AttachMemory(sd, (IPTR)sys_mem, virtual, length);
        }

        /* Set up a memory header to allocate from within graphics memory
         * window */
        chunk = (struct MemChunk *)(sd->Card.Framebuffer + virtual);
        chunk->mc_Next = NULL;
        chunk->mc_Bytes = length;

        header->mh_Node.ln_Type = NT_MEMORY;
        header->mh_Node.ln_Name = sys_mem;
        header->mh_First = chunk;
        header->mh_Lower = chunk;
        header->mh_Free = length;
        header->mh_Upper = (UBYTE *)chunk + length;
    }

    if (!success)
    {
        FreeVec(header);
        FreeAlignedMem(sys_mem);
        header = NULL;
    }

    return header;
}

void ReleaseGfxMemory(struct g45staticdata *sd, struct MemHeader *header)
{
    D(bug("[GMA] Releasing memory block to system (header=%p)\n", header));
    D(bug("[GMA] mh_Lower=%p mh_Upper=%p mh_Free=%p\n",
        header->mh_Lower, header->mh_Upper, header->mh_Free));

    G45_DetachMemory(sd, (char *)header->mh_Lower - sd->Card.Framebuffer,
        header->mh_Free);
    FreeAlignedMem(header->mh_Node.ln_Name);
}

static void G45_AttachMemory(struct g45staticdata *sd, intptr_t physical,
    intptr_t virtual, intptr_t length)
{
    intptr_t page = virtual >> 12;

    if (page > 0)
    {
        physical &= 0xfffff000;
        length &= 0xfffff000;

        do
        {
            writel(physical | 1, &sd->Card.GATT[page]);

            physical += 4096;
            length -= 4096;
            page++;
        }
        while ((page < sd->Card.GATT_size / 4) && length);
    }
    CacheClearU();
}

void G45_AttachCacheableMemory(struct g45staticdata *sd, intptr_t physical,
    intptr_t virtual, intptr_t length)
{
    intptr_t page = virtual >> 12;

    if (page > 0)
    {
        physical &= 0xfffff000;
        length &= 0xfffff000;

        do
        {
            writel(physical | 7, &sd->Card.GATT[page]);

            physical += 4096;
            length -= 4096;
            page++;
        }
        while ((page < sd->Card.GATT_size / 4) && length);
    }
    CacheClearU();
}

static void G45_DetachMemory(struct g45staticdata *sd, intptr_t virtual,
    intptr_t length)
{
    intptr_t page = virtual >> 12;

    if (page >= 0)
    {
        do
        {
            writel(0, &sd->Card.GATT[page]);

            length -= 4096;
            page++;
        }
        while ((page < sd->Card.GATT_size / 4) && length);
    }
    CacheClearU();
}

static APTR AllocAlignedMem(IPTR size, UWORD alignment)
{
    APTR mem = NULL, original_mem;

    size += 2 * sizeof(APTR) + alignment;
    original_mem = AllocMem(size, MEMF_REVERSE);
    if (original_mem != NULL)
    {
        mem = (APTR)((IPTR)(original_mem + 2 * sizeof(APTR) + alignment - 1)
            & ~(alignment - 1));
        *((APTR *)mem - 1) = original_mem;
        *((IPTR *)mem - 2) = size;
    }

    return mem;
}

static VOID FreeAlignedMem(APTR mem)
{
    if (mem != NULL)
        FreeMem(*((APTR *)mem - 1), *((IPTR *)mem - 2));

    return;
}

