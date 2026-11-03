/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native Amiga DOS integration regression: ordinary standard packets must
    use ordinary allocation direction. Exercise AllocDosObject itself with two
    controlled suitable holes and no other hole large enough for a packet.
*/
#include <aros/debug.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define ARENA_BYTES 4096
#define HOLE_BYTES 128
#define LOW_OFFSET 512
#define HIGH_OFFSET 3072
#define MAX_HELD 192

static ULONG checks, failures;

static void check(BOOL okay, CONST_STRPTR message)
{
    checks++;
    if (!okay)
    {
        failures++;
        bug("PACKET PLACEMENT FAIL: %s\n", message);
    }
}

int main(void)
{
    APTR held[MAX_HELD];
    ULONG sizes[MAX_HELD], heldCount = 0, largest = 0;
    UBYTE *arena;
    struct DosPacket *packet = NULL;
    BOOL holesFreed = FALSE, setupOkay = FALSE, low = FALSE, links = FALSE;
    BOOL packetAllocated = FALSE;
    ULONG minimum = (sizeof(struct StandardPacket) + sizeof(ULONG) + 7) & ~7UL;

    arena = AllocMem(ARENA_BYTES, MEMF_ANY);
    check(arena != NULL, "reserve controlled arena");
    if (!arena)
        goto done;

    /* No DOS I/O while memory is reserved. The operation under test only
     * calls AllocVec; quiet the other tasks while controlling its choices.
     */
    Forbid();
    while ((largest = AvailMem(MEMF_ANY | MEMF_LARGEST)) >= minimum)
    {
        if (heldCount == MAX_HELD)
            break;
        held[heldCount] = AllocMem(largest, MEMF_ANY | MEMF_NO_EXPUNGE);
        if (!held[heldCount])
            break;
        sizes[heldCount++] = largest;
    }
    setupOkay = largest < minimum;
    if (setupOkay)
    {
        /* Confirm the public allocator cannot satisfy another packet-sized
         * request before exposing the controlled holes. Do not reclaim caches.
         */
        APTR probe = AllocMem(minimum, MEMF_ANY | MEMF_NO_EXPUNGE);
        setupOkay = probe == NULL;
        if (probe)
            FreeMem(probe, minimum);
    }
    if (setupOkay)
    {
        FreeMem(arena + LOW_OFFSET, HOLE_BYTES);
        FreeMem(arena + HIGH_OFFSET, HOLE_BYTES);
        holesFreed = TRUE;
        packet = AllocDosObject(DOS_STDPKT, NULL);
        packetAllocated = packet != NULL;
        if (packet)
        {
            low = (UBYTE *)packet >= arena + LOW_OFFSET &&
                  (UBYTE *)packet < arena + LOW_OFFSET + HOLE_BYTES;
            links = packet->dp_Link &&
                packet->dp_Link->mn_Node.ln_Name == (char *)packet;
            FreeDosObject(DOS_STDPKT, packet);
        }
    }
    if (holesFreed)
    {
        FreeMem(arena, LOW_OFFSET);
        FreeMem(arena + LOW_OFFSET + HOLE_BYTES,
                HIGH_OFFSET - LOW_OFFSET - HOLE_BYTES);
        FreeMem(arena + HIGH_OFFSET + HOLE_BYTES,
                ARENA_BYTES - HIGH_OFFSET - HOLE_BYTES);
    }
    else
        FreeMem(arena, ARENA_BYTES);
    while (heldCount)
    {
        heldCount--;
        FreeMem(held[heldCount], sizes[heldCount]);
    }
    Permit();
    check(setupOkay, "only the two controlled holes can hold a packet");
    check(packetAllocated, "allocate a standard DOS packet");
    check(low, "ordinary standard packet selects the lower suitable hole");
    check(links, "standard packet has its public message link initialized");
done:
    bug("PACKET PLACEMENT TEST: %lu/%lu pass, %lu failures\n",
        checks - failures, checks, failures);
    return failures ? 20 : 0;
}
