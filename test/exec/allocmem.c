/*
    Copyright � 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

/*
 * Re-define output to bug in order to get exact measurements.
 * Console's history consumes memory, so this may look like a
 * memory leak.
 */
#define output printf

static inline void AccessTest(ULONG *ptr, BOOL trash)
{
    if (!trash)
        return;

    ptr[-1] = 0x40302010;	/* This should NOT cause mungwall warning */
    ptr[0]  = 0x01020304;	/* This SHOULD produce mungwall warning   */
}

static LONG test_allocabs(APTR block0, BOOL trash, BOOL leak, BOOL notlsf)
{
    LONG result = RETURN_OK;
    APTR start, block1;
    const ULONG allocsize = 4096;

    start = block0 + 1027;  /* Add some non-round displacement to make life harder */
    output("\nTesting AllocAbs(%lu, 0x%p) ...\n", (unsigned long)allocsize, start);
    if ((block1 = AllocAbs(allocsize, start)) != NULL)
    {
        output("Allocated at 0x%p, available memory: %lu bytes\n", block1, (unsigned long)AvailMem(MEMF_ANY));

        AccessTest(start + allocsize, trash);

        if (!leak)
        {
            output("Freeing the block at 0x%p of %lu bytes...\n", block1, (unsigned long)(allocsize + start - block1));
            FreeMem(block1, allocsize + start - block1);
            output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
        }
    }
    else
    {
        output("Allocation failed!\n");
        result = RETURN_ERROR;
    }

    /* Only perform the second AllocAbs() if leak isn't specified,
       otherwise we just duplicate the previous test */
    if (!leak)
    {
        output("\nTesting AllocAbs(%lu, 0x%p), but free using its requested start address...\n", (unsigned long)allocsize, start);
        if ((block1 = AllocAbs(allocsize, start)) != NULL)
        {
            output("Allocated at 0x%p, available memory: %lu bytes\n", block1, (unsigned long)AvailMem(MEMF_ANY));

            AccessTest(start + allocsize, trash);

            output("Freeing the block at 0x%p of %lu bytes...\n", start, (unsigned long)allocsize);
            if (notlsf)
            {
                FreeMem(start, allocsize);
                output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
            }
            else
            {
                FreeMem(block1, allocsize + start - block1);
                output("NOT SUPPORTED UNDER TLSF, freeing using returned address\n");
                output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
            }
        }
        else
        {
            output("Allocation failed!\n");
            result = RETURN_ERROR;
        }
    }

    return result;
}

static LONG test_allocpooled()
{
    LONG result = RETURN_OK;
    APTR pool, allocation;
    int allocstep;

    for (allocstep = 1; allocstep < 10; allocstep++)
    {
        output("\nTesting CreatePool(MEMF_CLEAR, %d, %d) ...\n", (allocstep << 10), (allocstep << 9));
        if ((pool = CreatePool(MEMF_CLEAR, (allocstep << 10), (allocstep << 9))) != NULL)
        {
            output("Allocated at 0x%p\n", pool);

            /* one that fits .. */
            output("\nTesting AllocPooled(0x%p, %d) ...\n", pool, (allocstep << 8));
            allocation = AllocPooled(pool, (allocstep << 8));
            if(allocation) {
                output("Allocated at 0x%p\n", pool);
            } else {
                output("Allocation failed!\n");
                result = RETURN_ERROR;
            }

            /* and one that doesnt .. */
            output("\nTesting AllocPooled(0x%p, %d) ...\n", pool, (allocstep << 11));
            allocation = AllocPooled(pool, (allocstep << 11));
            if(allocation) {
                output("Allocated at 0x%p\n", pool);
            } else {
                output("Allocation failed!\n");
                result = RETURN_ERROR;
            }

            output("Freeing the pool\n");
            DeletePool(pool);
            output("Done\n\n");
        }
        else
        {
             output("Allocation failed!\n\n");
        }
    }
    return result;
}


int main(int argc, char **argv)
{
    LONG result = RETURN_OK;
    int i;
    APTR block0;
    BOOL trash  = FALSE;
    BOOL leak   = FALSE;
    BOOL notlsf = FALSE;

    /*
     * Do some memory trashing if started with "trash" argument.
     * It's not adviced to do this without mungwall enabled.
     * The actual purpose of this is to test mungwall functionality.
     */
    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "trash"))
            trash = TRUE;
        else if (!strcmp(argv[i], "leak"))
            leak = TRUE;
        else if (!strcmp(argv[i], "notlsf"))
            notlsf = TRUE;

    }

    /* We Forbid() in order to see how our allocations influence free memory size */
    Forbid();

    output("Available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
    output("Largest chunk: %lu bytes\n\n", (unsigned long)AvailMem(MEMF_ANY|MEMF_LARGEST));

    output("Testing AllocMem(256 * 1024, MEMF_ANY) ...\n");
    if ((block0 = AllocMem(256 * 1024, MEMF_ANY)) != NULL)
    {
        output("Allocated at 0x%p, available memory: %lu bytes\n", block0, (unsigned long)AvailMem(MEMF_ANY));
        
        AccessTest(block0 + 256 * 1024, trash);
     
        if (!leak)
        {
            output("Freeing the block...\n");
            FreeMem(block0, 256 * 1024);
            output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
        }
    }
    else
    {
        output("Allocation failed!\n");
        result = RETURN_ERROR;
    }

    if(test_allocabs(block0, trash, leak, notlsf) != RETURN_OK)
        result = RETURN_ERROR;

    output("\nTesting AllocMem(4096, MEMF_ANY|MEMF_REVERSE) ...\n");
    if ((block0 = AllocMem(4096, MEMF_ANY|MEMF_REVERSE)) != NULL)
    {
        output("Allocated at 0x%p, available memory: %lu bytes\n", block0, (unsigned long)AvailMem(MEMF_ANY));

        /* This test actually proves that we don't hit for example MMIO region */
        *((volatile ULONG *)block0) = 0xC0DEBAD;
        if (*((volatile ULONG *)block0) != 0xC0DEBAD)
        {
            output("Invalid memory allocated!!!\n");
            result = RETURN_ERROR;
        }

        AccessTest(block0 + 4096, trash);

        if (!leak)
        {
            output("Freeing the block...\n");
            FreeMem(block0, 4096);
            output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
        }
    }
    else
    {
        output("Allocation failed!\n");
        result = RETURN_ERROR;
    }

    result = test_allocpooled();

    output("\nFinal available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));

    Permit();
    return result;
}
