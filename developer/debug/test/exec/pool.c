/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/exec.h>

#include <stdio.h>

/*
 * Re-define output to bug in order to get exact measurements.
 * Console's history consumes memory, so this may look like a
 * memory leak.
 */
#define output printf

int main(void)
{
    APTR chunks[200];
    APTR pool;
    int i;

    /* We Forbid() in order to see how our allocations influence free memory size */
    Forbid();

    output("Available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
    
    pool = CreatePool(MEMF_ANY, 4096, 4096);
    output("Created pool 0x%p, available memory: %lu bytes\n", pool, (unsigned long)AvailMem(MEMF_ANY));

    if (!pool)
    {
        Permit();
	output("Failed to create pool!\n");
	return 1;
    }

    output("Available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));

    output("Allocating 200 small chunks...\n");
    for (i = 0; i < 200; i++)
    {
	chunks[i] = AllocPooled(pool, 50);
	if (!chunks[i])
	{
	    output("Failed to allocate chunk %u!\n", i);
	    break;
	}
    }
    output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));

    output("Now freeing them...\n");
    for (i = 0; i < 200; i++)
    {
	if (!chunks[i])
	    break;
	FreePooled(pool, chunks[i], 50);
    }
    output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));

    output("Now allocating the whole puddle...\n");
    chunks[0] = AllocPooled(pool, 4096);
    output("Allocated at 0x%p, available memory: %lu bytes\n", chunks[0], (unsigned long)AvailMem(MEMF_ANY));

    output("Now allocating a BIG chunk...\n");
    chunks[1] = AllocPooled(pool, 16384);
    output("Allocated at 0x%p, available memory: %lu bytes\n", chunks[1], (unsigned long)AvailMem(MEMF_ANY));

    output("Freeing both chunks...\n");
    FreePooled(pool, chunks[0], 4096);
    FreePooled(pool, chunks[1], 16384);

    output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));

    output("Now attempting to re-allocate big chunk...\n");
    chunks[1] = AllocPooled(pool, 16384);
    output("Allocated at 0x%p, available memory: %lu bytes\n", chunks[1], (unsigned long)AvailMem(MEMF_ANY));
    
    output("Freeing the whole pool...\n");
    DeletePool(pool);
    output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
    
    Permit();
    return 0;
}
