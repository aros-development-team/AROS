/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/
#include <dos/dostags.h>
#include <dos/dos.h>
#include <proto/dos.h>

extern struct DosLibrary *DOSBase;

int main(void)
{
    BPTR segs;

    /* Load the boot shell */
    segs = LoadSeg ("c/shell");

    if (segs)
    {
	/* Execute it. */
	RunCommand (segs, AROS_STACKSIZE, "FROM S:Startup-Sequence", 23);
	UnLoadSeg (segs);
    }

    return 0;
} /* main */
