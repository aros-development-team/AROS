#include <aros/debug.h>
#include <proto/exec.h>

#include <stdio.h>
#include <string.h>

/*
 * Re-define output to bug in order to get exact measurements.
 * Console's history consumes memory, so this may look like a
 * memory leak.
 */
#define output bug

int main(int argc, char **argv)
{
    APTR block0, start, block1;
    BOOL trash = FALSE;

    /*
     * Do some memory trashing if started with "trash" argument.
     * It's not adviced to do this without mungwall enabled
     */
    if ((argc > 1) && (!strcmp(argv[1], "trash")))
    	trash = TRUE;

    /* We Forbid() in order to see how our allocations influence free memory size */
    Forbid();

    output("Available memory: %u bytes\n", AvailMem(MEMF_ANY));

    output("Allocating 256 KB...\n");
    block0 = AllocMem(256 * 1024, MEMF_ANY);
    output("Allocated at 0x%p, available memory: %u bytes\n", block0, AvailMem(MEMF_ANY));
    
    if (trash)
    	*(ULONG *)(block0 + 256 * 1024) = 0x01020304;

    output("Freeing the block...\n");
    FreeMem(block0, 256 * 1024);
    output("Done, available memory: %u bytes\n", AvailMem(MEMF_ANY));

    start = block0 + 1024;
    output("Now trying AllocAbs() 4 KB at 0x%p\n", start);
    block1 = AllocAbs(4096, start);
    output("Allocated at 0x%p, available memory: %u bytes\n", block1, AvailMem(MEMF_ANY));

    if (trash)
    	*(ULONG *)(block1 + 4096) = 0x01020304;

    output("Freeing the block...\n");
    FreeMem(block1, 4096 + start - block1);
    output("Done, available memory: %u bytes\n", AvailMem(MEMF_ANY));
    
    Permit();
    return 0;
}
