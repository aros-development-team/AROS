/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    AllocAbs() for addresses that cannot be given must return NULL.
*/

#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include <stdio.h>

#define BLOCKSIZE 65536
#define ASKSIZE   4096

static int failures;

static void expect(BOOL ok, const char *what, APTR result)
{
    printf("%s: %s (0x%p)\n", what, ok ? "ok" : "FAILED", result);
    if (!ok)
        failures++;
}

int main(void)
{
    UBYTE *block = AllocMem(BLOCKSIZE, MEMF_ANY);
    APTR mem;
    int i;

    if (!block)
    {
        printf("No memory for the test\n");
        return RETURN_FAIL;
    }

    /* Inside a block that is in use: there is nothing to give. */
    for (i = 0; i < 8; i++)
    {
        mem = AllocAbs(ASKSIZE, block + ASKSIZE + i * ASKSIZE);
        expect(mem == NULL, "address in an allocated block", mem);
        if (mem)
            break;      /* the block is damaged now, do not go on with it */
    }

    /* The same place once the block is free. */
    if (!failures)
    {
        FreeMem(block, BLOCKSIZE);
        mem = AllocAbs(ASKSIZE, block + ASKSIZE);
        expect(mem != NULL && (UBYTE *)mem <= block + ASKSIZE, "address in a free block", mem);
        if (mem)
            FreeMem(mem, ASKSIZE + (block + ASKSIZE - (UBYTE *)mem));
    }

    printf("%s\n", failures ? "Test FAILED" : "Test passed");
    return failures ? RETURN_FAIL : RETURN_OK;
}
