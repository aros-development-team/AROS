/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Show the use or Ressource Tracking (RT)
    Lang: english
*/

#define ENABLE_RT   1 /* Enable RT */

#include <stdio.h>
#include <aros/rt.h>
#include <exec/memory.h>
#include <proto/exec.h>

int main (int argc, char ** argv)
{
    APTR mem, illmem;
    int size;

    /*
	Init resource tracking. This will be a NOP if ENABLE_RT is 0 or
	undefined.
    */
    RT_Init();

    /* 1. Allocate some memory */
    printf ("Allocating %d bytes of memory\n", size=60);

    mem = AllocMem (size, MEMF_ANY);

    /* Bug 1: Ooops... forgetting about mem */
    printf ("Bug 1: Forgetting about %d bytes at %p\n", size, mem);
    mem = AllocMem (size, MEMF_ANY);

    printf ("Got some %d bytes at %p\n", size, mem);

    /*
	Bug 2: Wrong size. Take note that RT will *not* free the memory.
	This will take place when RT_Exit() is called.
    */
    printf ("Bug 2: Freeing %d bytes at %p\n", size+10, mem);

    FreeMem (mem, size+10);

    /* Bug 3: Wrong pointer */
    illmem = (APTR)4;
    printf ("Bug 3: Freeing %d bytes at %p\n", size, illmem);

    FreeMem (illmem, size);

    /*
	Terminate RT. This will also be a NOP if RT is not enabled. If
	any resources are still allocated, then this will free and print
	them.
    */
    printf ("Show Bugs 1 and 2\n");
    RT_Exit ();

    return 0;
}
