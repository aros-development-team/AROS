
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/vmm.h>

#include <stdio.h>
#include <stdlib.h>

int main (void)
{
    struct Library *VMMBase;
    APTR buffer;

    if ((VMMBase = OpenLibrary ("vmm.library", 0L)) == NULL)
    {
        printf ("Couldn't open vmm.library\n");
        exit (5);
    }

    printf ("VM available: %ld\n", AvailVMem (0L));

    buffer = AllocVMem (1000L, MEMF_CLEAR);
    printf ("Allocated mem at %lx\n", buffer);
    printf ("VM available: %ld\n", AvailVMem (0L));

    FreeVMem (buffer, 1000L);
    printf ("VM available: %ld\n", AvailVMem (0L));

    printf ("Largest block: %ld\n", AvailVMem (MEMF_LARGEST));
    CloseLibrary (VMMBase);
    exit (0);
}
