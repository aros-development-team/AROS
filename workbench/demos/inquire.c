/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Use of aros.library/ArosInquire()
    Lang:
*/

#include <aros/inquire.h>
#include <aros/arosbase.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/aros.h>

#include <stdio.h>

static const char version[]= "$VER: inquire 41.1 (29.3.1997)\n\r";

struct Library *ArosBase;

int main(int argc, char **argv)
{
    int rc;
    ULONG relMajor, relMinor, vers;
    ULONG kickbase, kicksize;
    UWORD kickver, kickrev;

    rc = 0;

    if(!(ArosBase = OpenLibrary(AROSLIBNAME, AROSLIBVERSION)))
    {
	FPrintf((BPTR)stderr, "Couldn't open "AROSLIBNAME"\n");
	return RETURN_FAIL;
    }

    if(AROSLIBREVISION < ArosBase->lib_Revision)
    {
	CloseLibrary(ArosBase);
	FPrintf((BPTR)stderr,
	    AROSLIBNAME" is too old! Need at least version %ld.%ld\n",
		(ULONG)AROSLIBVERSION, (ULONG)AROSLIBREVISION);
	return RETURN_FAIL;
    }

    ArosInquire(
	AI_ArosVersion, 	(IPTR)&vers,
	AI_ArosReleaseMajor,	(IPTR)&relMajor,
	AI_ArosReleaseMinor,	(IPTR)&relMinor,
	AI_KickstartBase,	(IPTR)&kickbase,
	AI_KickstartSize,	(IPTR)&kicksize,
	AI_KickstartVersion,	(IPTR)&kickver,
	AI_KickstartRevision,	(IPTR)&kickrev,
	TAG_DONE);

    Printf("AROS release = %ld.%ld\n", relMajor, relMinor);
    Printf("AROS module major version = V%ld\n", vers);

    if (kickbase)
    {
	Printf("Kickstart base address = $%lx\n", kickbase);

	Printf("Kickstart size = $%lx (%ld kB)\n", kicksize, kicksize/1024);

	Printf("Kickstart version = %d.%d\n", kickver, kickrev);
    }
    else
    {
	Printf("This machine has no Kickstart ROM.\n");
    }

    CloseLibrary(ArosBase);

    return 0;
}
