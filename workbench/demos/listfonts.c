/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Demo/test of diskfont/AvailFonts()
    Lang: English.
*/


#include <proto/diskfont.h>
#include <proto/exec.h>
#include <diskfont/diskfont.h>
#include <exec/memory.h>
#include <stdio.h>

#include <proto/dos.h>

#define DEBUG 1
#include <aros/debug.h>

struct Library *DiskfontBase;

int main(int argc, char ** argv)
{
    ULONG afshortage;

    struct AvailFontsHeader *afh;
    struct AvailFonts *afptr;

    /* Try to guess how many bytes are needed */
    ULONG afsize	= 10000;

    if (!(DiskfontBase = OpenLibrary("diskfont.library", 0L)))
    {
	FPrintf((BPTR)stderr, "Couldn't open diskfont.library\n");
	return (0);
    }

    do
    {
	afh = (struct AvailFontsHeader *)AllocMem(afsize, MEMF_ANY);
	if (afh)
	{
	    afshortage = AvailFonts((STRPTR)afh, afsize, AFF_MEMORY);
	    if (afshortage)
	    {
		FreeMem(afh, afsize);
		afsize += afshortage;
		afh = (struct AvailFontsHeader*)(-1L);
	    }
	}
    } while (afshortage && afh);

    if (afh)
    {
	/* Print some info about the fonts */
	UWORD count;

	Printf("Number of fonts found: %d\n", afh->afh_NumEntries);

	/* Get pointer to the first AvailFonts item */
	afptr = (struct AvailFonts*)&afh[1];

	for (count = afh->afh_NumEntries; count --;)
	{
	    Printf
	    (
		"Font name: %s\t\t\tFont YSize: %d\n",
		(LONG)afptr->af_Attr.ta_Name,
		afptr->af_Attr.ta_YSize
	    );

	    afptr ++;
	}
    }

    CloseLibrary(DiskfontBase);
    return (0);
}
