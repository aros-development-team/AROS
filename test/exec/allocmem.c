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

static BOOL trash = FALSE;
static BOOL leak  = FALSE;

static inline void AccessTest(ULONG *ptr)
{
    if (!trash)
	return;

    ptr[-1] = 0x40302010;	/* This should NOT cause mungwall warning */
    ptr[0]  = 0x01020304;	/* This SHOULD produce mungwall warning   */
}

int main(int argc, char **argv)
{
    int i;
    APTR block0, start, block1;

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
    }

    /* We Forbid() in order to see how our allocations influence free memory size */
    Forbid();

    output("Available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));

    output("Allocating 256 KB...\n");
    block0 = AllocMem(256 * 1024, MEMF_ANY);
    output("Allocated at 0x%p, available memory: %lu bytes\n", block0, (unsigned long)AvailMem(MEMF_ANY));
    
    AccessTest(block0 + 256 * 1024);
 
    if (!leak)
    {
    	output("Freeing the block...\n");
    	FreeMem(block0, 256 * 1024);
    	output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
    }

    start = block0 + 1027;	/* Add some none-round displacement to make the life harder */
    output("Now trying AllocAbs() 4 KB at 0x%p\n", start);
    block1 = AllocAbs(4096, start);
    output("Allocated at 0x%p, available memory: %lu bytes\n", block1, (unsigned long)AvailMem(MEMF_ANY));

    AccessTest(start + 4096);

    if (!leak)
    {
    	output("Freeing the block...\n");
    	FreeMem(block1, 4096 + start - block1);
    	output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
    }

    output("Now repeat this AllocAbs(), but free using our requested start address...\n");
    block1 = AllocAbs(4096, start);
    output("Allocated at 0x%p, available memory: %lu bytes\n", block1, (unsigned long)AvailMem(MEMF_ANY));

    if (block1)
    {
	AccessTest(start + 4096);

	if (!leak)
	{
	    output("Freeing the block...\n");
	    FreeMem(start, 4096);
	    output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
	}
    }

    output("And now trying MEMF_REVERSE...\n");
    block0 = AllocMem(4096, MEMF_REVERSE);
    output("Allocated at 0x%p, available memory: %lu bytes\n", block0, (unsigned long)AvailMem(MEMF_ANY));
 
    /* This test actually proves that we don't hit for example MMIO region */
    *((volatile ULONG *)block0) = 0xC0DEBAD;
    if (*((volatile ULONG *)block0) != 0xC0DEBAD)
        output("It's not a memory!!!\n");
 
    AccessTest(block0 + 4096);
 
    if (!leak)
    {
    	output("Freeing the block...\n");
    	FreeMem(block0, 4096);
    	output("Done, available memory: %lu bytes\n", (unsigned long)AvailMem(MEMF_ANY));
    }

    Permit();
    return 0;
}
