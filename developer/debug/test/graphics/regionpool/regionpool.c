/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#define HOLES 8
#define HOLE_SIZE 16384
#define FRAGMENTED_REGIONS 32
#define REGIONS 64
#define RESERVATIONS 256

struct Reservation
{
    APTR memory;
    ULONG size;
};

static struct Reservation reserved[RESERVATIONS];
static APTR holes[HOLES], guards[HOLES];
static struct Region *regions[REGIONS];
static volatile ULONG reclaim_calls;
static ULONG failures;

static AROS_INTH1(count_reclaims, APTR, unused)
{
    AROS_INTFUNC_INIT
    reclaim_calls++;
    return MEM_DID_NOTHING;
    AROS_INTFUNC_EXIT
}

static void check(BOOL condition, CONST_STRPTR message)
{
    if (!condition)
    {
        bug("REGION POOL FAIL: %s\n", message);
        failures++;
    }
}

static BOOL contains(struct Region *region, WORD x, WORD y)
{
    struct RegionRectangle *rr;
    for (rr = region->RegionRectangle; rr; rr = rr->Next)
        if (x >= region->bounds.MinX + rr->bounds.MinX &&
            x <= region->bounds.MinX + rr->bounds.MaxX &&
            y >= region->bounds.MinY + rr->bounds.MinY &&
            y <= region->bounds.MinY + rr->bounds.MaxY)
            return TRUE;
    return FALSE;
}

int main(void)
{
    struct Interrupt handler = {0};
    struct Rectangle point = {0, 0, 0, 0};
    ULONG i, n = 0, largest, initial_free, after_free, fragmented_free;
    BOOL installed = FALSE, exhausted = FALSE;

    /* The fixture intentionally bounds all available memory. Run on a
     * stock CD32, with only Chip RAM, rather than consuming expansion RAM. */
    if (AvailMem(MEMF_FAST | MEMF_TOTAL))
    {
        bug("REGION POOL SKIP: requires a Chip-RAM-only machine\n");
        return 5;
    }
    initial_free = AvailMem(MEMF_CHIP);
    for (i = 0; i < REGIONS; i++)
    {
        regions[i] = NewRegion();
        if (!regions[i])
            goto setup_failed;
    }
    for (i = 0; i < HOLES; i++)
    {
        /* Allocate each hole and its adjacent guard together. Separate
         * allocations could put guards in unrelated small free ranges. */
        holes[i] = AllocMem(HOLE_SIZE + 256, MEMF_CHIP | MEMF_NO_EXPUNGE);
        if (!holes[i])
            goto setup_failed;
        guards[i] = (UBYTE *)holes[i] + HOLE_SIZE;
    }
    /* Keep allocated guards between the holes. Consume the remaining large
     * free ranges without invoking any low-memory handler during setup. */
    while ((largest = AvailMem(MEMF_CHIP | MEMF_LARGEST)) >= 512)
    {
        if (n == RESERVATIONS)
            goto setup_failed;
        reserved[n].memory = AllocMem(largest, MEMF_CHIP | MEMF_NO_EXPUNGE);
        if (!reserved[n].memory)
            goto setup_failed;
        reserved[n++].size = largest;
    }
    for (i = 0; i < HOLES; i++)
    {
        FreeMem(holes[i], HOLE_SIZE);
        holes[i] = NULL;
    }
    check(AvailMem(MEMF_CHIP | MEMF_LARGEST) < 32768,
          "fixture has no large contiguous free block");

    handler.is_Node.ln_Type = NT_INTERRUPT;
    handler.is_Node.ln_Pri = 127;
    handler.is_Node.ln_Name = "region pool pressure regression";
    handler.is_Code = (APTR)count_reclaims;
    AddMemHandler(&handler);
    installed = TRUE;
    fragmented_free = AvailMem(MEMF_CHIP);

    /* Several live regions require more than one reduced pool. Their
     * geometry must be correct without expunging unrelated caches. */
    for (i = 0; i < FRAGMENTED_REGIONS; i++)
    {
        struct Rectangle right = {2, 0, 2, 0}, bottom = {0, 2, 0, 2};
        WORD x, y;
        check(OrRectRegion(regions[i], &point), "first point survives fragmentation");
        check(OrRectRegion(regions[i], &right), "second point survives fragmentation");
        check(OrRectRegion(regions[i], &bottom), "third point survives fragmentation");
        for (y = 0; y < 3; y++)
            for (x = 0; x < 3; x++)
                check(contains(regions[i], x, y) ==
                      ((y == 0 && (x == 0 || x == 2)) || (x == 0 && y == 2)),
                      "region contains exactly the requested points");
    }
    check(reclaim_calls == 0, "optional pool reservation must not invoke reclamation");
    bug("REGION POOL FRAGMENTED: %lu reclaim calls\n", (unsigned long)reclaim_calls);
    for (i = 0; i < FRAGMENTED_REGIONS; i++)
        ClearRegion(regions[i]);
    check(AvailMem(MEMF_CHIP) == fragmented_free,
          "all reduced pools are released when their regions become empty");

    /* One region also crosses a rectangle-chunk boundary. Check its actual
     * geometry after repeated list replacement, then release both chunks. */
    if (!failures)
    {
        for (i = 0; i < 140; i++)
        {
            struct Rectangle row = {0, i * 2, 0, i * 2};
            check(OrRectRegion(regions[0], &row), "region grows across chunk boundaries");
        }
        for (i = 0; i < 280; i++)
            check(contains(regions[0], 0, i) == !(i & 1),
                  "multi-chunk region retains every requested row");
        ClearRegion(regions[0]);
        check(AvailMem(MEMF_CHIP) == fragmented_free,
              "multi-chunk region returns all pool memory");
        check(reclaim_calls == 0, "multi-chunk region needs no cache reclamation");
    }

    /* A genuinely exhausted allocator must still use normal reclamation
     * for its minimum allocation, then report failure cleanly. */
    reclaim_calls = 0;
    for (i = 0; i < REGIONS; i++)
        if (!OrRectRegion(regions[i], &point))
        {
            exhausted = TRUE;
            break;
        }
    check(exhausted, "real exhaustion reports allocation failure");
    check(reclaim_calls != 0, "minimum allocation still invokes reclamation");
    goto out;

setup_failed:
    check(FALSE, "prepare fragmented-memory fixture");
out:
    if (installed)
        RemMemHandler(&handler);
    for (i = 0; i < REGIONS; i++)
        if (regions[i])
            DisposeRegion(regions[i]);
    for (i = 0; i < n; i++)
        FreeMem(reserved[i].memory, reserved[i].size);
    for (i = 0; i < HOLES; i++)
    {
        if (holes[i])
            FreeMem(holes[i], HOLE_SIZE + 256);
        else if (guards[i])
            FreeMem(guards[i], 256);
    }
    after_free = AvailMem(MEMF_CHIP);
    check(after_free >= initial_free, "all region and fixture memory is returned");
    bug("REGION POOL MEMORY: before %lu after %lu\n",
        (unsigned long)initial_free, (unsigned long)after_free);
    bug("REGION POOL TEST: %lu failures\n", (unsigned long)failures);
    return failures ? 20 : 0;
}
