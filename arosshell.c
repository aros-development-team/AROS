/*
    (C) 1995-96 AROS - The Amiga Research OS
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

#define CANNOT_OPEN_INTUITION	"Cannot open intuition.library\n"
#define CANNOT_LOAD_SHELL	"Unable to load C:shell\n"

int main(void)
{
    BPTR segs;

    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary ("intuition.library",39)) )
    {
	Write (Output (), CANNOT_OPEN_INTUITION, sizeof(CANNOT_OPEN_INTUITION)-1);
	return 20;
    }

    /* Load the boot shell */
    segs = LoadSeg ("C:shell");

    if (segs)
    {
	/* Execute it. */
	RunCommand (segs, AROS_STACKSIZE, "FROM S:Startup-Sequence", 23);
	UnLoadSeg (segs);
    }
    else
    {
	Write (Output (), CANNOT_LOAD_SHELL, sizeof(CANNOT_LOAD_SHELL)-1);
    }

    CloseLibrary ((struct Library *)IntuitionBase);

    return 0;
} /* main */
