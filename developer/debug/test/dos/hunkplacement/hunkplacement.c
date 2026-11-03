/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Native Amiga integration test for LoadSeg allocation direction. Two free
    regions can hold a synthetic executable; its requested allocation direction
    must select the appropriate region. RunCommand also executes the loaded code.
*/

#include <aros/debug.h>
#include <dos/doshunks.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define HUNK_BYTES (192 * 1024)
#define HOLE_BYTES (256 * 1024)
#define OS_RESERVE (128 * 1024)
#define MAX_HELD 32

static ULONG checks, failures;
static ULONG commandStack;

static void check(BOOL okay, CONST_STRPTR message)
{
    checks++;
    if (!okay)
    {
        failures++;
        bug("HUNK PLACEMENT FAIL: %s\n", message);
    }
}

static BOOL fixture(CONST_STRPTR path, ULONG memory)
{
    ULONG image[14];
    ULONG count = 0;
    BPTR file;
    BOOL okay;

    image[count++] = HUNK_HEADER;
    image[count++] = 0;
    image[count++] = 1;
    image[count++] = 0;
    image[count++] = 0;
    image[count++] = HUNK_BYTES / 4 | memory;
    if (memory == (HUNKF_CHIP | HUNKF_FAST))
        image[count++] = MEMF_PUBLIC | MEMF_CHIP | MEMF_CLEAR | MEMF_REVERSE;
    image[count++] = HUNK_CODE;
    image[count++] = 3;
    /* move.l a7,commandStack; moveq #42,d0; rts; padding */
    image[count++] = 0x23cf0000 | ((ULONG)&commandStack >> 16);
    image[count++] = ((ULONG)&commandStack << 16) | 0x702a;
    image[count++] = 0x4e750000;
    image[count++] = HUNK_END;

    file = Open(path, MODE_NEWFILE);
    if (!file)
        return FALSE;
    okay = Write(file, image, count * sizeof(ULONG)) == count * sizeof(ULONG);
    Close(file);
    return okay;
}

int main(void)
{
    static CONST_STRPTR paths[] = {
        "RAM:hunkplacement-any", "RAM:hunkplacement-chip",
        "RAM:hunkplacement-reverse"
    };
    static const ULONG memory[] = {0, HUNKF_CHIP, HUNKF_CHIP | HUNKF_FAST};
    APTR held[MAX_HELD], arena = NULL;
    ULONG sizes[MAX_HELD], heldCount = 0, arenaSize = 0, largest, i;
    BOOL holesFreed = FALSE;

    for (i = 0; i < 3; i++)
    {
        BOOL okay = fixture(paths[i], memory[i]);
        check(okay, "create executable fixture");
        if (!okay)
            goto done;
    }

    largest = AvailMem(MEMF_CHIP | MEMF_LARGEST);
    check(largest >= 3 * HOLE_BYTES + OS_RESERVE,
          "Chip RAM has room for controlled allocation regions");
    if (largest < 3 * HOLE_BYTES + OS_RESERVE)
        goto done;

    arenaSize = (largest - OS_RESERVE) & ~7UL;
    arena = AllocMem(arenaSize, MEMF_CHIP);
    check(arena != NULL, "reserve allocation arena");
    if (!arena)
        goto done;

    /* Leave small blocks for DOS/filesystem activity, but prevent a large
     * executable from fitting anywhere except the two regions below.
     */
    while ((largest = AvailMem(MEMF_CHIP | MEMF_LARGEST)) >= HUNK_BYTES)
    {
        if (heldCount == MAX_HELD)
            break;
        held[heldCount] = AllocMem(largest, MEMF_CHIP);
        if (!held[heldCount])
            break;
        sizes[heldCount++] = largest;
    }
    check(largest < HUNK_BYTES, "other free regions cannot hold the executable");
    if (largest >= HUNK_BYTES)
        goto done;

    FreeMem(arena, HOLE_BYTES);
    FreeMem((UBYTE *)arena + arenaSize - HOLE_BYTES, HOLE_BYTES);
    holesFreed = TRUE;

    for (i = 0; i < 3; i++)
    {
        BPTR segment = LoadSeg(paths[i]);
        UBYTE *code = segment ? (UBYTE *)BADDR(segment) + sizeof(BPTR) : NULL;
        BOOL low = code && code >= (UBYTE *)arena &&
            code < (UBYTE *)arena + HOLE_BYTES;
        BOOL high = code && code >= (UBYTE *)arena + arenaSize - HOLE_BYTES;

        check(segment != BNULL, "load executable into controlled free region");
        if (!segment)
            continue;
        bug("HUNK PLACEMENT: %s loaded at %p; low=%ld\n", paths[i], code, (LONG)low);
        check(i == 2 ? high : low,
              i == 2 ? "explicit reverse request selects upper region" :
                       "ordinary hunk selects lower region");
        check(RunCommand(segment, 8192, "\n", 1) == 42,
              "loaded executable returns its result");
        check(commandStack >= (ULONG)arena &&
              commandStack < (ULONG)arena + HOLE_BYTES,
              "command stack selects lower suitable region");
        UnLoadSeg(segment);
    }

done:
    if (arena)
    {
        if (holesFreed)
            FreeMem((UBYTE *)arena + HOLE_BYTES, arenaSize - 2 * HOLE_BYTES);
        else
            FreeMem(arena, arenaSize);
    }
    while (heldCount)
    {
        heldCount--;
        FreeMem(held[heldCount], sizes[heldCount]);
    }
    for (i = 0; i < 3; i++)
        DeleteFile(paths[i]);
    bug("HUNK PLACEMENT TEST: %lu/%lu pass, %lu failures\n",
        checks - failures, checks, failures);
    return failures ? 20 : 0;
}
