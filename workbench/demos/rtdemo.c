/*
    Copyright © 1997, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Show the use or Ressource Tracking (RT)
    Lang: english
*/
#define ENABLE_RT   1 /* Enable RT */

#include <stdio.h>

#include <exec/memory.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <aros/rt.h>

struct IntuitionBase * IntuitionBase;

int main (int argc, char ** argv)
{
    APTR mem, illmem;
    BPTR file;
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

    /* Allocate some more memory */
    mem = AllocVec (size, MEMF_ANY);
    printf ("Bug 4: Forget to free %d bytes at %p\n", size, mem);

    /* Bug 5: Wrong pointer */
    illmem = (APTR)4;
    printf ("Bug 5: Freeing %p\n", illmem);

    FreeVec (illmem);

    /* This is valid */
    FreeVec (NULL);

    mem = IntuitionBase = (struct IntuitionBase *) OpenLibrary (INTUITIONNAME, 0);

    printf ("Bug 12: Open a window and forget about it\n");
    OpenWindowTags (NULL, TAG_END);

    printf ("Bug 6: OverWriting IntuitionBase (%p)\n", IntuitionBase);
    IntuitionBase = (struct IntuitionBase *)23;

    CloseLibrary ((struct Library *) IntuitionBase);

    IntuitionBase = mem;

    /* Funny things with files */
    printf ("Bug 7: Open a file with an illegal mode\n");
    file = Open ("test", 0xbad);

    printf ("Bug 8: Open a file with an illegal name\n");
    file = Open (NULL, MODE_NEWFILE);

    printf ("Bug 9: Open a file which doesn't exist and forget to check\n");
    file = Open ("XG&hg", MODE_OLDFILE);

    Read (file, mem, size);

    printf ("Bug 10: Write with an illegal buffer\n");
    file = Open ("test.rtdemo", MODE_NEWFILE);

    Write (file, NULL, size);

    printf ("Bug 11: Close the wrong file\n");
    file = (BPTR)mem;
    Close (file);

    DeleteFile ("test.rtdemo");

    /*
	Terminate RT. This will also be a NOP if RT is not enabled. If
	any resources are still allocated, then this will free and print
	them.
    */
    printf ("Show Bugs 1, 2, 6, 11 and 12\n");
    RT_Exit ();

    return 0;
}
