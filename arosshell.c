/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: This is the "boot code" of AROS when it runs as an emulation.
    Lang: english
*/
#include <dos/dostags.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>
#include <proto/dos.h>
#include <proto/exec.h>

extern struct DosLibrary *DOSBase;
struct IntuitionBase * IntuitionBase;

int main(void)
{
    BPTR segs;

    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary ("intuition.library",39)) )
    {
	Write (Output (), "Cannot open intuition.library\n", 32);
	return 20;
    }

    /* Load the boot shell */
    segs = LoadSeg ("c/shell");

    if (segs)
    {
	/* Execute it. */
	RunCommand (segs, AROS_STACKSIZE, "FROM S:Startup-Sequence", 23);
	UnLoadSeg (segs);
    }

    CloseLibrary ((struct Library *)IntuitionBase);

    return 0;
} /* main */
