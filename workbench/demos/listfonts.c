/*
    Copyright © 1995-96, The AROS Development Team. All rights reserved.
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
    ULONG afshortage = 0;
    IPTR  pargs[5];

    struct AvailFontsHeader *afh;
    struct AvailFonts *afptr;

    /* Try to guess how many bytes are needed */
    ULONG afsize	= 10000;

    if (!(DiskfontBase = OpenLibrary("diskfont.library", 0L)))
    {
	VPrintf ("Couldn't open diskfont.library\n", NULL);
	return (RETURN_FAIL);
    }

    do
    {
	afh = (struct AvailFontsHeader *)AllocMem(afsize, MEMF_ANY);
	if (afh)
	{
	    afshortage = AvailFonts((STRPTR)afh, afsize, AFF_MEMORY|AFF_DISK);
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

	pargs[0] = afh->afh_NumEntries;
	VPrintf("Number of fonts found: %ld\n", pargs);

	/* Get pointer to the first AvailFonts item */
	afptr = (struct AvailFonts*)&afh[1];

	for (count = afh->afh_NumEntries; count; count --)
	{
	    pargs[0] = afptr->af_Type;
	    pargs[1] = (IPTR)afptr->af_Attr.ta_Name;
	    pargs[2] = afptr->af_Attr.ta_YSize;
    	    pargs[3] = afptr->af_Attr.ta_Style;
	    pargs[4] = afptr->af_Attr.ta_Flags;
	    
	    VPrintf ("[%ld] Font name: %-30.s Font YSize: %ld  Style: 0x%lx  Flags 0x%lx\n", pargs);

	    afptr ++;
	}
    }

    CloseLibrary(DiskfontBase);
    return (RETURN_OK);
}
